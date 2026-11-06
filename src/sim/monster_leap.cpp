// Walk-jumpers: the leap over a gap of pits and traps. Where it lands: Dungeon::leapLanding (dungeon_walkers.cpp).
#include "monster.h"
#include <algorithm>
#include <cmath>
#include "player.h"
#include "../level/journal.h"
#include "../level/gameplay_config.h"
#include "../level/head_bump.h"
#include "render_config.h"

bool Monster::canJump() const {
	return leaps(type->locomotion) && Alive() && !jumping() && GameClock::now() >= leap.readyMs;
}

bool Monster::canPounce() const {
	return type->pounce && canJump() && !RollingBall() && !InWater() && GameClock::now() >= leap.pounceReadyMs;
}

void Monster::Pounce(float toX, int dir) {
	if (!type->pounce)
		return;
	Jump(toX);
	leap.pounceDir = dir;
	leap.pounceReadyMs = GameClock::now() + type->pounce->cooldownMs;
}

void Monster::Jump(float toX) {
	int now = GameClock::now();
	leap.fromX = x;
	leap.toX = toX;
	leap.startMs = now;
	leap.readyMs = now + MONSTER_JUMP_COOLDOWN_MS;
	leap.lift = 0.f;
	leap.bumpAir = -1.f;
	leap.bumpStunMs = 0;
	leap.pounceDir = 0;
	leap.catchCol = -1;
	enter(ModelState::Jump);
	sound(CharacterSound::Jump);
	links.journal->SeeMove(type->id, links.level, CreatureMove::Leap);
}

void Monster::LeapToRungs(int ladderCol, int toRow) {
	const float air = type->isBoss() ? BOSS_LEAP_CATCH_AIR : LEAP_CATCH_AIR;
	const auto middle = static_cast<float>(ladderCol - col); // tile-local
	Jump(x + (middle - x) / air); // over the ladder's middle at that share of the flight
	leap.catchCol = ladderCol;
	leap.catchToRow = toRow;
	leap.catchAir = air;
}

LeapBump Monster::UpdateJump(float px, float room) {
	if (leap.pounceDir != 0 && type->pounce) { // the player stepped towards it: it lands short of them, not in them
		const auto dir = static_cast<float>(leap.pounceDir);
		const float stop = HalfWidth() + links.player->HalfWidth() + type->pounce->landGap;
		const float limit = px - dir * stop - static_cast<float>(Col()) - 0.5f;
		leap.toX = leap.pounceDir > 0 ? std::min(leap.toX, limit) : std::max(leap.toX, limit);
	}
	float t = static_cast<float>(GameClock::now() - leap.startMs) / static_cast<float>(MONSTER_JUMP_MS);
	if (t >= 1.f) {
		x = leap.toX;
		leap.lift = 0.f;
		leap.startMs = -1;
		if (Alive()) {
			enter(ModelState::Move);
			if (leap.bumpStunMs > 0)
				StunHit(leap.bumpStunMs, false);
		}
		leap.bumpStunMs = 0;
		return LeapBump::None;
	}
	float air = std::clamp((t - MONSTER_JUMP_TAKEOFF) / (MONSTER_JUMP_TOUCHDOWN - MONSTER_JUMP_TAKEOFF), 0.f, 1.f);
	x = leap.fromX + (leap.toX - leap.fromX) * air;
	const float arc = 4.f * MONSTER_JUMP_HEIGHT * air * (1.f - air);
	LeapBump bump = LeapBump::None;
	if (leap.bumpAir < 0.f && air < 0.5f && arc > room) { // the head meets the ceiling, still rising
		leap.bumpAir = air;
		leap.bumpLift = std::max(0.f, room);
		// The arc's rise speed is 1 - 2 air of the take-off's: a hard bump above HEAD_BUMP_MIN_RISE_FRACTION of it.
		bump = 1.f - 2.f * air > HEAD_BUMP_MIN_RISE_FRACTION ? LeapBump::Hard : LeapBump::Knock;
	}
	if (leap.bumpAir >= 0.f) { // the rise stopped: down from there
		const float u = std::min(1.f, (air - leap.bumpAir) / (1.f - leap.bumpAir));
		leap.lift = leap.bumpLift * (1.f - u * u);
	} else
		leap.lift = std::min(arc, room);
	if (leap.catchCol >= 0 && air >= leap.catchAir) { // over the ladder: the paws catch the rungs
		const float from = leap.lift / RenderConfig::TILE_SIZE;
		const int ladderCol = leap.catchCol;
		leap.catchCol = -1;
		leap.lift = 0.f;
		leap.startMs = -1;
		if (Alive()) {
			StartClimb(ladderCol, leap.catchToRow, from);
			if (leap.bumpStunMs > 0) // a bump on the way: dazed on the rungs
				StunHit(leap.bumpStunMs, false);
		} else
			x = static_cast<float>(ladderCol - col); // killed in the air: it drops down the shaft from there
		leap.bumpStunMs = 0;
	}
	return bump;
}

int Monster::HeadBump(Rng& rng) {
	if (!Alive())
		return 0;
	const HeadBumpDaze roll = rollHeadBumpDaze(rng);
	const int lost = vitals.HeadBumpLoss(MaxHealth());
	sound(CharacterSound::Attack); // the grunt (no kind has a hurt sound)
	leap.bumpMemoryMs = GameClock::now() + MONSTER_HEAD_BUMP_MEMORY_MS;
	takeHit(lost); // a kill gives no XP: Dungeon::UpdateMonsters sees it dead, not the player's
	if (Alive() && roll.daze)
		leap.bumpStunMs = roll.ms;
	return lost;
}

int Monster::TrapCrossHits() const {
	const float tilesPerSecond = std::max(0.01f, WalkStep() * 1000.f / static_cast<float>(MONSTER_STEP_PERIOD_MS));
	const float way = 1.f + 2.f * HalfWidth() * (1.f - TRAP_BOX_FORGIVE);
	return static_cast<int>(std::ceil(way / tilesPerSecond * 1000.f / static_cast<float>(TRAP_HURT_INTERVAL_MS)));
}

int Monster::CeilingSpikes(bool death, Rng& rng) {
	if (!Alive())
		return 0;
	const int before = vitals.hp;
	sound(CharacterSound::Attack); // the grunt, as a bump's
	leap.bumpMemoryMs = GameClock::now() + MONSTER_HEAD_BUMP_MEMORY_MS;
	TrapHit(TrapHurt::rampTotal(TrapCrossHits()));
	if (death)
		TakePoison(PoisonTier::Strong, false, rng);
	return before - vitals.hp;
}
