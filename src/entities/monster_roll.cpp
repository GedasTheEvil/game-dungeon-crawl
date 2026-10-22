// Rollers (the giant scarab, Khepri; plan scarab-dung-ball): the windup with its back to the player, the kick. The
// ball's roll is the dungeon's (world/dungeon_balls.cpp).
#include "monster.h"
#include <algorithm>
#include "../core/gameplay_config.h"
#include "../world/journal.h"

bool Monster::canRoll(float px, float py) const {
	if (!type->roll || !Alive() || !alerted || lurking() || RollingBall() || jumping() ||
		GameClock::now() < roll.nextMs || attackDirection(px, py) == 0)
		return false;
	const float gap = PlayerGap(px);
	return gap >= BallRules().minRange && gap <= BallRules().maxRange;
}

void Monster::StartRoll(float px) {
	roll.startMs = GameClock::now();
	roll.dir = px < CentreX() ? -1 : 1;
	roll.kicked = false;
	fleeDir = 0;
	enter(ModelState::AttackRanged);
	links.journal->SeeMove(type->id, links.level, CreatureMove::RollBall);
}

bool Monster::UpdateRoll() {
	if (!RollingBall())
		return false;
	const RollRules rules = BallRules();
	const int age = GameClock::now() - roll.startMs;
	if (!Alive() || age >= rules.windupMs + DUNG_BALL_RECOVER_MS) { // killed in the windup: no ball
		roll.startMs = -1;
		return false;
	}
	if (roll.kicked || age < rules.windupMs)
		return false;
	roll.kicked = true;
	roll.nextMs = GameClock::now() + rules.cooldownMs;
	sound(CharacterSound::AttackRanged);
	return true;
}

float Monster::BallShaped() const {
	if (!RollingBall() || roll.kicked)
		return 0.f;
	return std::clamp(static_cast<float>(GameClock::now() - roll.startMs) /
						  static_cast<float>(std::max(1, BallRules().windupMs)),
					  0.f, 1.f);
}

float Monster::BallX() const { return CentreX() + static_cast<float>(roll.dir) * HalfWidth(); }
