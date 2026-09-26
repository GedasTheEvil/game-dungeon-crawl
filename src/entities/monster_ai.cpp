#include "monster.h"
#include <algorithm>
#include <cmath>
#include <stdio.h>
// // #include "stats.h"
#include "../state/game_state.h"
#include "../input/gameplay_config.h"
#include "../core/service_locator.h"

int monster::attackDirection() {
	if (!speed) {
		if (*dungeonCamX - tileOriginX - 0.5 > 0.2 + 0.02 * scale && std::fabs(tileOriginY - *dungeonCamY) < 0.8)
			return 1;
		else if (*dungeonCamX - tileOriginX - 0.5 < -0.2 - 0.02 * scale && std::fabs(tileOriginY - *dungeonCamY) < 0.8)
			return -1;
		else
			return 0;
	}

	if ((mapX + tileOriginX + 0.5) - *dungeonCamX > 0.05 + 0.02 * scale && std::fabs(tileOriginY - *dungeonCamY) < 0.8)
		return -1;
	else if ((mapX + tileOriginX + 0.5) - *dungeonCamX < -0.05 - 0.02 * scale &&
			 std::fabs(tileOriginY - *dungeonCamY) < 0.8)
		return 1;
	return 0;
}

int monster::Seek() {
	if (Alive()) {
		mapX += MONSTER_SEEK_STEP * (attackDirection() * speed);

		if (!attackDirection())
			return 0;
		else
			applyModelState(ModelState::Move);

		return 1;
	}

	return 0;
}

void monster::Attack() {
	if (!Alive())
		return;

	applyModelState(ModelState::Attack);

	if (/*Att_timer -> TimePassed() &&*/ std::fabs(tileOriginY - *dungeonCamY) < 0.8) {
		GAME_STATE.ui.Stats->GetHit(damage);
		att_s.Play();
	}
}

void monster::GetCords(float& xx, float& yy) {
	xx = mapX;
	yy = mapY;
}

bool monster::Nearby(float xx, float yy, int rangei) {

	float range = 0.1f * static_cast<float>(rangei);

	if (fabs(tileOriginX + mapX + 0.5 - xx) <= range && std::fabs(tileOriginY - yy) < 0.7)
		return 1;

	return 0;
}

void monster::setModelState(ModelState state) { applyModelState(state); }

int monster::Model_state() { return static_cast<int>(currentState); }

float monster::flightProbeX() const {
	int dir = flight.dir;
	if (flight.phase == FlightPhase::Return)
		dir = mapX > 0 ? -1 : 1;
	return tileOriginX + mapX + 0.5f + static_cast<float>(dir) * BAT_WALL_MARGIN;
}

void monster::Fly(bool wallAhead) {
	const int now = GameClock::now();
	const float dt = flight.lastMs < 0 ? 0.f : static_cast<float>(std::min(now - flight.lastMs, 100)) / 1000.f;
	flight.lastMs = now;
	const float roost = BAT_CEILING - idleTop * scale;
	if (flight.lift < 0)
		flight.lift = roost;

	if (!Alive()) { // drops to the floor
		flight.fall += BAT_FALL_GRAVITY * dt;
		flight.lift = std::max(0.f, flight.lift - flight.fall * dt);
		return;
	}

	const float dx = *dungeonCamX - (tileOriginX + mapX + 0.5f); // to the player
	const bool sees = std::fabs(tileOriginY - *dungeonCamY) < 0.8f && std::fabs(dx) <= BAT_SIGHT;
	const float step = BAT_TILES_PER_SPEED * static_cast<float>(speed) * dt;
	float targetLift = roost;

	switch (flight.phase) {
	case FlightPhase::Roost:
		if (!sees) {
			applyModelState(ModelState::Idle);
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
			if (std::fabs(tileOriginY - *dungeonCamY) < 0.8f) {
				GAME_STATE.ui.Stats->GetHit(damage);
				att_s.Play();
			}
		}
		if (wallAhead || ahead < -BAT_OVERSHOOT) {
			flight.dir = -flight.dir;
			flight.bitten = false;
			if (!sees && std::fabs(dx) > BAT_OVERSHOOT + 0.5f) // lost the player (climbed away, ran off)
				flight.phase = FlightPhase::Return;
		} else
			mapX += static_cast<float>(flight.dir) * step;
		// Swoops down to the player, climbs away from him.
		const float k = std::min(std::fabs(dx) / BAT_OVERSHOOT, 1.f);
		const float low = BAT_LOW_LIFT + BAT_WING_DIP * scale;
		targetLift = low + (std::max(BAT_HIGH_LIFT, low) - low) * k * k * (3 - 2 * k);
		applyModelState(now < flight.attackUntilMs ? ModelState::Attack : ModelState::Move);
		break;
	}
	case FlightPhase::Return:
		if (sees) {
			flight.phase = FlightPhase::Swoop;
			flight.dir = dx >= 0 ? 1 : -1;
			break;
		}
		flight.dir = mapX > 0 ? -1 : 1;
		if (std::fabs(mapX) <= step) {
			mapX = 0;
			if (std::fabs(flight.lift - roost) < 1.f)
				flight.phase = FlightPhase::Roost;
		} else if (!wallAhead)
			mapX += static_cast<float>(flight.dir) * step;
		applyModelState(ModelState::Move);
		break;
	}
	flight.lift += (targetLift - flight.lift) * std::min(1.f, BAT_LIFT_RATE * dt);
}
