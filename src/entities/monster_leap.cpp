// Walk-jumpers: the leap over a gap of pits and traps. Where it lands: Dungeon::leapLanding (dungeon_walkers.cpp).
#include "monster.h"
#include <algorithm>
#include "player.h"
#include "../world/journal.h"
#include "../core/gameplay_config.h"

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
	leap.pounceDir = 0;
	enter(ModelState::Jump);
	sound(CharacterSound::Jump);
	links.journal->SeeMove(type->id, links.level, CreatureMove::Leap);
}

void Monster::UpdateJump(float px) {
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
		if (Alive())
			enter(ModelState::Move);
		return;
	}
	float air = std::clamp((t - MONSTER_JUMP_TAKEOFF) / (MONSTER_JUMP_TOUCHDOWN - MONSTER_JUMP_TAKEOFF), 0.f, 1.f);
	x = leap.fromX + (leap.toX - leap.fromX) * air;
	leap.lift = 4.f * MONSTER_JUMP_HEIGHT * air * (1.f - air);
}
