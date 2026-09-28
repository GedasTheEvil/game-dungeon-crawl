#include "monster.h"
#include <algorithm>
#include <cmath>
#include "../state/game_state.h"
#include "../input/gameplay_config.h"
#include "../core/service_locator.h"

int Monster::attackDirection(float px, float py) const {
	const float scale = type->scale;
	const auto tileX = static_cast<float>(col);
	if (!sameRow(py))
		return 0;
	if (type->locomotion == Locomotion::Stationary) {
		if (px - tileX - 0.5 > 0.2 + 0.02 * scale)
			return 1;
		if (px - tileX - 0.5 < -0.2 - 0.02 * scale)
			return -1;
		return 0;
	}

	if ((x + tileX + 0.5) - px > 0.05 + 0.02 * scale)
		return -1;
	if ((x + tileX + 0.5) - px < -0.05 - 0.02 * scale)
		return 1;
	return 0;
}

bool Monster::Seek(bool blocked, float px, float py) {
	if (!Alive())
		return false;
	int dir = attackDirection(px, py);
	if (dir == 0)
		return false;

	if (!blocked)
		x += MONSTER_SEEK_STEP * static_cast<float>(dir * type->speed);
	enter(ModelState::Move);
	return true;
}

void Monster::Attack(float py) {
	if (!Alive())
		return;

	enter(ModelState::Attack);
	if (sameRow(py)) {
		GAME_STATE.ui.stats->TakeHit(type->damage);
		type->model.attackSound.Play();
	}
}

float Monster::seekProbeX(int dir) const { return CentreX() + static_cast<float>(dir) * MONSTER_WALL_MARGIN; }

float Monster::flightProbeX() const {
	int dir = flight.dir;
	if (flight.phase == FlightPhase::Return)
		dir = x > 0 ? -1 : 1;
	return CentreX() + static_cast<float>(dir) * BAT_WALL_MARGIN;
}

void Monster::Fly(bool wallAhead, float px, float py) {
	const float scale = type->scale;
	const int now = GameClock::now();
	const float dt = flight.lastMs < 0 ? 0.f : static_cast<float>(std::min(now - flight.lastMs, 100)) / 1000.f;
	flight.lastMs = now;
	const float roost = BAT_CEILING - type->model.idleTop * scale;
	if (flight.lift < 0)
		flight.lift = roost;

	if (!Alive()) { // drops to the floor
		flight.fall += BAT_FALL_GRAVITY * dt;
		flight.lift = std::max(0.f, flight.lift - flight.fall * dt);
		return;
	}

	const float dx = px - CentreX(); // to the player
	const bool sees = sameRow(py) && std::fabs(dx) <= BAT_SIGHT;
	const float step = BAT_TILES_PER_SPEED * static_cast<float>(type->speed) * dt;
	float targetLift = roost;

	switch (flight.phase) {
	case FlightPhase::Roost:
		if (!sees) {
			enter(ModelState::Idle);
			return;
		}
		flight.phase = FlightPhase::Swoop;
		flight.dir = dx >= 0 ? 1 : -1;
		flight.bitten = false;
		[[fallthrough]];
	case FlightPhase::Swoop: {
		const float ahead = dx * static_cast<float>(flight.dir); // > 0: the player is still in front
		if (!flight.bitten && ahead <= BAT_BITE_REACH && ahead > -BAT_OVERSHOOT) {
			flight.bitten = true;
			flight.attackUntilMs = now + BAT_ATTACK_MS;
			if (sameRow(py)) {
				GAME_STATE.ui.stats->TakeHit(type->damage);
				type->model.attackSound.Play();
			}
		}
		if (wallAhead || ahead < -BAT_OVERSHOOT) {
			flight.dir = -flight.dir;
			flight.bitten = false;
			if (!sees && std::fabs(dx) > BAT_OVERSHOOT + 0.5f) // lost the player (climbed away, ran off)
				flight.phase = FlightPhase::Return;
		} else
			x += static_cast<float>(flight.dir) * step;
		// Swoops down to the player, climbs away from them.
		const float k = std::min(std::fabs(dx) / BAT_OVERSHOOT, 1.f);
		const float low = BAT_LOW_LIFT + BAT_WING_DIP * scale;
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
}

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
	type->model.jumpSound.Play();
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
