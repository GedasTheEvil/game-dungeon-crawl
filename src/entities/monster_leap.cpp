// Walk-jumpers: the leap over a gap of pits and traps. Where it lands: Dungeon::leapLanding (dungeon_walkers.cpp).
#include "monster.h"
#include <algorithm>
#include "player.h"
#include "../world/journal.h"
#include "../core/gameplay_config.h"
#include "../world/head_bump.h"

bool Monster::canJump() const {
	return leaps(type->locomotion) && health > 0 && !jumping() && GameClock::now() >= leap.readyMs;
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
	enter(ModelState::Jump);
	sound(CharacterSound::Jump);
	links.journal->SeeMove(type->id, links.level, CreatureMove::Leap);
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
	return bump;
}

int Monster::HeadBump(Rng& rng) {
	if (!Alive())
		return 0;
	const HeadBumpDaze roll = rollHeadBumpDaze(rng);
	const int lost = std::min(headBumpHp(MaxHealth()), health);
	sound(CharacterSound::Attack); // the grunt (no kind has a hurt sound)
	leap.bumpMemoryMs = GameClock::now() + MONSTER_HEAD_BUMP_MEMORY_MS;
	takeHit(lost); // a kill gives no XP: Dungeon::UpdateMonsters sees it dead, not the player's
	if (Alive() && roll.daze)
		leap.bumpStunMs = roll.ms;
	return lost;
}
