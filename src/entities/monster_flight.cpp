// Flyers (bats): roost, swoop, return, and the way to another row (plan bats-high-ceiling). The level's part:
// Dungeon::flightGoal (dungeon_flight.cpp).
#include "monster.h"
#include <algorithm>
#include <cmath>
#include "player.h"
#include "../world/journal.h"
#include "../core/gameplay_config.h"
#include "../graphics/render_config.h"

float Monster::flightProbeX() const {
	int dir = flight.dir;
	if (flight.phase == FlightPhase::Return)
		dir = x > 0 ? -1 : 1;
	return CentreX() + static_cast<float>(dir) * BAT_WALL_MARGIN;
}

float Monster::roostLift() const { return BAT_CEILING - type->model.idleTop * type->scale; }

bool Monster::screech(bool inCone, int now) {
	const std::optional<ScreechRules>& rules = type->screech;
	if (!rules)
		return false;
	if (flight.tellUntilMs < 0) {
		if (!inCone || flight.bitten || now < flight.screechReadyMs)
			return false;
		flight.tellUntilMs = now + rules->tellMs;
		flight.screechReadyMs = now + rules->cooldownMs;
		sound(CharacterSound::Roar); // the bat's roar is its screech: the rising squeal, the blast at the tell's end
		links.journal->SeeMove(type->id, links.level, CreatureMove::Screech);
		return true;
	}
	if (now < flight.tellUntilMs)
		return true;
	flight.tellUntilMs = -1;
	if (inCone) { // still in it: no step out of the cone, no wall between
		links.player->TakeHit(rules->damage, SCREECH_MIX, *links.events); // the blast hurts (plan screech-bite)
		links.player->Daze(rules->dazeMs, *links.events);
	}
	return false;
}

void Monster::Fly(bool wallAhead, float px, float py, const FlightGoal& goal) {
	const float scale = type->scale;
	const int now = GameClock::now();
	const float dt = flight.lastMs < 0 ? 0.f : static_cast<float>(std::min(now - flight.lastMs, 100)) / 1000.f;
	flight.lastMs = now;
	const float roost = roostLift();
	if (flight.lift < 0)
		flight.lift = roost;

	if (!Alive()) { // drops to the floor, on through open cells to the one under it
		flight.fall += BAT_FALL_GRAVITY * dt;
		flight.lift -= flight.fall * dt;
		if (flight.lift < 0.f && goal.openBelow) {
			row--;
			flight.lift += RenderConfig::TILE_SIZE;
		}
		flight.lift = std::max(0.f, flight.lift);
		return;
	}
	if (Reeling()) { // stunned (plan skill-stun): drops to its row's floor as a dead one, lies there, then flies on
		flight.fall += BAT_FALL_GRAVITY * dt;
		flight.lift = std::max(0.f, flight.lift - flight.fall * dt);
		return;
	}
	flight.fall = 0.f;

	const float dx = px - CentreX(); // to the player
	const bool sees = goal.sees;
	const float step = BAT_TILES_PER_SPEED * type->speed * dt;
	float targetLift = roost;
	// On the way to another row: along this one to the shaft, then up or down it, facing the player.
	auto changeRow = [&]() {
		if (std::fabs(goal.shaftX - x) > step) {
			flight.dir = goal.shaftX > x ? 1 : -1;
			x += static_cast<float>(flight.dir) * step;
			targetLift = BAT_HIGH_LIFT;
		} else {
			x = goal.shaftX;
			flight.dir = dx >= 0 ? 1 : -1;
			targetLift = goal.toRow < row ? -BAT_HIGH_LIFT : RenderConfig::TILE_SIZE + BAT_HIGH_LIFT;
		}
		flight.bitten = false;
		enter(ModelState::Move);
	};

	if (goal.flee) { // up off the roost and away from the player, on its row; Return once it sees them again
		flight.phase = FlightPhase::Return;
		if (goal.fleeDir != 0) {
			if (flight.dir == goal.fleeDir && !wallAhead)
				x += static_cast<float>(flight.dir) * step;
			flight.dir = goal.fleeDir;
		}
		targetLift = BAT_HIGH_LIFT;
		enter(ModelState::Move);
	} else
		switch (flight.phase) {
		case FlightPhase::Roost:
			if (!sees && flight.lift < roost - 1.f) { // dropped from the ceiling, no one to hunt: back up
				enter(ModelState::Move);
				break;
			}
			if (!sees) {
				enter(ModelState::Idle);
				return;
			}
			flight.phase = FlightPhase::Swoop;
			flight.dir = dx >= 0 ? 1 : -1;
			flight.bitten = false;
			alerted = true;
			links.journal->SeeMove(type->id, links.level, CreatureMove::Swoop);
			[[fallthrough]];
		case FlightPhase::Swoop: {
			if (goal.toRow != row) {
				if (sees)
					changeRow();
				else
					flight.phase = FlightPhase::Return;
				break;
			}
			if (screech(goal.inCone, now)) { // stops in the air through the tell
				targetLift = flight.lift;
				enter(ModelState::Move);
				break;
			}
			const float ahead = dx * static_cast<float>(flight.dir); // > 0: the player is still in front
			const float above = py - static_cast<float>(row); // the player's feet up the rungs of a ladder
			if (!flight.bitten && ahead <= BAT_BITE_REACH && ahead > -BAT_OVERSHOOT) {
				flight.bitten = true;
				flight.attackUntilMs = now + BAT_ATTACK_MS;
				if (Reaches(px, py, flight.dir))
					bite();
			}
			if (wallAhead || ahead < -BAT_OVERSHOOT) {
				flight.dir = -flight.dir;
				flight.bitten = false;
				if (!sees && std::fabs(dx) > BAT_OVERSHOOT + 0.5f) // lost the player (climbed away, ran off)
					flight.phase = FlightPhase::Return;
			} else
				x += static_cast<float>(flight.dir) * step;
			// Swoops down to the player (to their head up a ladder too), climbs away from them.
			const float k = std::min(std::fabs(dx) / BAT_OVERSHOOT, 1.f);
			const float low = std::min(
				BAT_LOW_LIFT + BAT_WING_DIP * scale + std::max(above, 0.f) * RenderConfig::TILE_SIZE,
				roost
			);
			targetLift = low + (std::max(BAT_HIGH_LIFT, low) - low) * k * k * (3 - 2 * k);
			enter(now < flight.attackUntilMs ? ModelState::Attack : ModelState::Move);
			break;
		}
		case FlightPhase::Return:
			if (sees) {
				flight.phase = FlightPhase::Swoop;
				flight.dir = dx >= 0 ? 1 : -1;
				break;
			}
			if (goal.toRow != row) { // back to the roost's row first
				changeRow();
				break;
			}
			flight.dir = x > 0 ? -1 : 1;
			if (std::fabs(x) <= step) {
				x = 0;
				if (std::fabs(flight.lift - roost) < 1.f)
					flight.phase = FlightPhase::Roost;
			} else if (!wallAhead)
				x += static_cast<float>(flight.dir) * step;
			enter(ModelState::Move);
			break;
		}
	flight.lift += (targetLift - flight.lift) * std::min(1.f, BAT_LIFT_RATE * dt);
	// Through the floor into the row under it, or through the ceiling into the one above.
	if (flight.lift < 0.f && goal.toRow < row) {
		row--;
		flight.lift += RenderConfig::TILE_SIZE;
	} else if (flight.lift >= RenderConfig::TILE_SIZE && goal.toRow > row) {
		row++;
		flight.lift -= RenderConfig::TILE_SIZE;
	}
}
