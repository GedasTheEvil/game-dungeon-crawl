// Walk-jumpers: the leap over a gap of pits and traps. Where it lands: Dungeon::leapLanding (dungeon_walkers.cpp).
#include "monster.h"
#include <algorithm>
#include "../world/journal.h"
#include "../core/gameplay_config.h"

bool Monster::canJump() const {
	return type->locomotion == Locomotion::WalkJump && health > 0 && !jumping() && GameClock::now() >= leap.readyMs;
}

void Monster::Jump(float toX) {
	int now = GameClock::now();
	leap.fromX = x;
	leap.toX = toX;
	leap.startMs = now;
	leap.readyMs = now + MONSTER_JUMP_COOLDOWN_MS;
	leap.lift = 0.f;
	enter(ModelState::Jump);
	sound(CharacterSound::Jump);
	links.journal->SeeMove(type->id, links.level, CreatureMove::Leap);
}

void Monster::UpdateJump() {
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
