#include "monster.h"
#include <cmath>
#include <stdio.h>
// // #include "stats.h"
#include "../state/game_state.h"
#include "../input/gameplay_config.h"
#include "../core/service_locator.h"

int monster::attackDirection() {
	if (!speed) {
		if (*dungeonX - X - 0.5 > 0.2 + 0.02 * scale && std::fabs(Y - *dungeonY) < 0.8)
			return 1;
		else if (*dungeonX - X - 0.5 < -0.2 - 0.02 * scale && std::fabs(Y - *dungeonY) < 0.8)
			return -1;
		else
			return 0;
	}

	if ((x + X + 0.5) - *dungeonX > 0.05 + 0.02 * scale && std::fabs(Y - *dungeonY) < 0.8)
		return -1;
	else if ((x + X + 0.5) - *dungeonX < -0.05 - 0.02 * scale && std::fabs(Y - *dungeonY) < 0.8)
		return 1;
	return 0;
}

int monster::Seek() {
	if (Alive()) {
		x += MONSTER_SEEK_STEP * (attackDirection() * speed);

		if (!attackDirection())
			return 0;
		else
			model = walk.get();

		return 1;
	}

	return 0;
}

void monster::Attack() {
	if (!Alive())
		return;

	model = attack.get();

	if (/*Att_timer -> TimePassed() &&*/ std::fabs(Y - *dungeonY) < 0.8) {
		GAME_STATE.ui.Stats->GetHit(damage);
		att_s.Play();
	}
}

void monster::GetCords(float& xx, float& yy) {
	xx = x;
	yy = y;
}

bool monster::Nearby(float xx, float yy, int rangei) {

	float range = 0.1f * static_cast<float>(rangei);

	if (fabs(X + x + 0.5 - xx) <= range && std::fabs(Y - yy) < 0.7)
		return 1;

	return 0;
}

void monster::changeMDL(int id) {
	if (id == 0) {
		model = walk.get();
		return;
	}

	if (id == 1) {
		model = attack.get();
		return;
	}

	if (id == 2) {
		model = die.get();
		return;
	}
}

int monster::Model_state() {
	if (model == walk.get())
		return 1;
	if (model == attack.get())
		return 2;
	return 0;
}

void monster::setModel(int state) {
	if (state == 1)
		model = walk.get();
	else if (state == 2)
		model = attack.get();
	else
		model = die.get();
}
