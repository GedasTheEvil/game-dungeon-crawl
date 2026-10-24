// Dung balls on the level (plan scarab-dung-ball; the roller's windup and kick: entities/monster_roll.cpp): the ball
// rolls along the floor of its row over traps, falls into a pit, sinks in water, breaks on a wall, at the end of its
// roll or on the player. A jump clears it.
#include "dungeon.h"
#include "../entities/player.h"
#include "journal.h"
#include "rng.h"
#include "world_events.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float BALL_STEP_S = 0.004f; // the hit test's interval along the roll
} // namespace

bool Dungeon::updateRoller(Monster& mon) {
	if (mon.RollingBall()) {
		if (mon.UpdateRoll())
			balls.push_back(
				{mon.BallX(),
				 static_cast<float>(mon.Row()),
				 mon.BallDir(),
				 mon.BallRules(),
				 static_cast<int>(mon.Type()->id),
				 GameClock::now()}
			);
		return true;
	}
	if (won || mon.OnRungs() || !mon.canRoll(mapX, mapY) || !clearRow(mon.CentreX(), mapX, mon.Row()))
		return false;
	mon.StartRoll(mapX);
	return true;
}
//======================================================================================
void Dungeon::breakBall(DungBall& ball, bool inWater) {
	ball.breakMs = GameClock::now();
	ball.sank = inWater;
	sim.events->Play(inWater ? WorldSound::Splash : WorldSound::DungThud);
}
//======================================================================================
void Dungeon::updateBalls() {
	const int now = GameClock::now();
	Player& player = *sim.player;
	const float half = player.HalfWidth();
	const float height = player.Height();
	for (auto it = balls.begin(); it != balls.end();) {
		DungBall& b = *it;
		if (b.breakMs >= 0) {
			it = now - b.breakMs >= DUNG_BALL_CRUMBLE_MS ? balls.erase(it) : it + 1;
			continue;
		}
		const RollRules& rules = b.rules;
		const float r = rules.radius;
		bool gone = false;
		for (float left = static_cast<float>(UPDATE_TICK_MS) / 1000.f; left > 0.f && b.breakMs < 0 && !gone;
			 left -= BALL_STEP_S) {
			const float dt = std::min(left, BALL_STEP_S);
			const auto col = static_cast<int>(std::floor(b.x));
			if (b.falling) { // straight down the pit, to the floor under it
				b.vy -= DUNG_BALL_FALL_GRAVITY * dt;
				b.y += b.vy * dt;
				const auto cell = static_cast<int>(std::floor(b.y));
				if (!IsInBounds(col, cell)) {
					gone = true;
				} else if (inHalfWater(MapAt(col, cell))) {
					breakBall(b, true);
				} else if (isSolidTile(MapAt(col, cell))) {
					b.y = static_cast<float>(cell + 1);
					breakBall(b, false);
				}
			} else {
				b.x += static_cast<float>(b.dir) * rules.speed * dt;
				b.rolled += rules.speed * dt;
				const auto row = static_cast<int>(std::floor(b.y));
				const auto ahead = static_cast<int>(std::floor(b.x + static_cast<float>(b.dir) * r));
				const auto under = static_cast<int>(std::floor(b.x));
				if (!IsInBounds(ahead, row) || isSolidTile(MapAt(ahead, row)) || b.rolled >= DUNG_BALL_MAX_ROLL)
					breakBall(b, false);
				else if (inHalfWater(MapAt(under, row)))
					breakBall(b, true);
				else if (!IsInBounds(under, row - 1) || !isSolidTile(MapAt(under, row - 1)))
					b.falling = true;
			}
			if (gone || b.breakMs >= 0 || !player.Alive() || std::fabs(b.x - mapX) > half + r || b.y >= mapY + height ||
				b.y + 2.f * r <= mapY)
				continue;
			// On the player: over the ball in a jump, else hit (or sidestepped), and it breaks on them.
			if (!player.Dodges(*sim.events, sim.random->gameplay)) {
				player.TakeHit(rules.damage, rules.mix, *sim.events, false, rules.burnMs);
				sim.journal->HitByCreature(b.from, levelNumber);
			}
			breakBall(b, false);
		}
		it = gone ? balls.erase(it) : it + 1;
	}
}
