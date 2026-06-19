#include "monster.h"
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
			applyModelState(ModelState::Walk);

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

void monster::changeMDL(int id) { applyModelState(static_cast<ModelState>(id)); }

int monster::Model_state() { return static_cast<int>(currentState); }

void monster::setModel(int state) { applyModelState(static_cast<ModelState>(state)); }
