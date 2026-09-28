#include "player.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "../state/game_state.h"
#include "../graphics/ink.h"
#include "../test/scenario.h"

bool Player::Load(const char* name, const Texture& texture) {
	if (!model.Load(name, texture, PLAYER_CLIPS))
		return false;
	for (AnimPlayback& p : playback)
		p.stepStart = GameClock::now();
	state = model.Reference();
	return true;
}

void Player::Draw() {
	glPushMatrix();
	glTranslatef(0, 0, -30 + depthOffset);
	glPushMatrix(); // will add rotation
	glScalef(scale, scale, scale);

	auto drawBlood = [&] {
		glPushMatrix();
		glScalef(0.5f / scale, 0.5f / scale, 0.5f / scale);
		Game().assets.textures.nullTex.Bind();
		blood.Explode();
		blood.Fall();
		blood.Draw();
		glPopMatrix();
	};
	if (Alive()) {
		if (state == ModelState::Die) // healed back to life
			setModelState(ModelState::Idle);
		drawBlood();
	} else
		setModelState(ModelState::Die);
	drawBlood(); // even when dead

	model.BindTexture();
	glRotatef(rotA, 0, 1, 0);
	const float figure = Ink::figureScale();
	glScalef(figure, figure, figure);
	model.Show(state, playback);

	glPopMatrix();
	glPopMatrix();
	if (state != ModelState::Climb) // the climb frame follows the height (showClimb)
		model.Advance(state, playback);
}

void Player::TakeHit(int dmg, bool ignoreArmor) {
	if (Scenario::godMode())
		return;
	const int s = static_cast<int>(scale);
	if (Alive()) {
		stats.LoseHP(stats.HitDamage(dmg, ignoreArmor));
		blood.setCords(static_cast<float>(random() % s), static_cast<float>(random() % s), 0);
		blood.Reset();
	}

	if (!Alive() && state != ModelState::Die) {
		setModelState(ModelState::Die);
		model.dieSound.Play();

		blood.setCords(static_cast<float>(random() % s), static_cast<float>(random() % s), 0);
		blood.Reset();
		for (int i = 0; i < 6; i++)
			blood.Explode();
	}
}

void Player::Reanimate() {
	stats.HealFully();
	setModelState(model.Reference());
}

void Player::showClimb(float phase) {
	const AnimatedModel* climb = model.Clip(ModelState::Climb);
	if (!climb)
		return;
	state = ModelState::Climb;
	int frames = climb->FrameCount();
	float frame = std::min(phase - std::floor(phase), 1.f) * static_cast<float>(frames);
	playback[static_cast<int>(ModelState::Climb)] = {std::min(frame, static_cast<float>(frames) - 0.001f), 0};
}
