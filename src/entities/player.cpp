#include "player.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "../state/game_state.h"
#include "../graphics/ink.h"
#include "../graphics/render_config.h"
#include "../test/scenario.h"

namespace {
constexpr float DEG_TO_RAD = 3.14159265f / 180.f;
} // namespace

bool Player::Load(const char* name, const Texture& texture) {
	if (!model.Load(name, texture, PLAYER_CLIPS))
		return false;
	for (AnimPlayback& p : playback)
		p.stepStart = GameClock::now();
	state = model.Reference();
	findFists();
	return true;
}

// The fists are raised in front of the chest in every clip: the most forward corners (+z) at 50-85 % of the
// idle height, one on each side.
void Player::findFists() {
	const AnimatedModel* idle = model.Clip(ModelState::Idle);
	if (!idle)
		return;
	const auto [low, high] = idle->YRange(0);
	std::array<float, 2> forward{-1e9f, -1e9f};
	for (int i = 0; i < idle->VertexCount(); i++) {
		const std::array<float, 3> v = idle->Vertex(0, i);
		const float up = (v[1] - low) / (high - low);
		const int side = v[0] < 0 ? 0 : 1;
		if (up > 0.5f && up < 0.85f && v[2] > forward[side]) {
			forward[side] = v[2];
			fists[side] = i;
		}
	}
}

std::array<float, 3> Player::Fist(int dir) const {
	const float s = scale * Ink::figureScale();
	const int fist = fists[dir > 0 ? 0 : 1]; // turned right, the model's -x side is towards the camera
	const AnimatedModel* clip = model.Clip(model.Shown(state));
	std::array<float, 3> v{0, 0.75f, 0.2f}; // no fists found: in front of the chest (the model is 1 tall)
	if (fist >= 0 && fist < clip->VertexCount() && shownFrame < clip->FrameCount())
		v = clip->Vertex(shownFrame, fist);
	// As Draw(): moved back, scaled, turned rotA round y.
	const float a = rotA * DEG_TO_RAD;
	return {s * (v[0] * std::cos(a) + v[2] * std::sin(a)), s * v[1],
			-30.f + depthOffset + s * (-v[0] * std::sin(a) + v[2] * std::cos(a))};
}

float Player::HalfWidth() const { return model.HalfWidth() * scale * Ink::figureScale() / RenderConfig::TILE_SIZE; }

float Player::Height() const { return model.referenceTop * scale * Ink::figureScale() / RenderConfig::TILE_SIZE; }

// The blood runs twice a tick while alive (and is drawn twice), once when dead: as it always has.
void Player::Animate() {
	if (Alive()) {
		if (state == ModelState::Die) // healed back to life
			setModelState(ModelState::Idle);
		blood.Explode();
		blood.Fall();
	} else
		setModelState(ModelState::Die);
	blood.Explode();
	blood.Fall();
	if (state != ModelState::Climb) // the climb frame follows the height (showClimb)
		model.Advance(state, playback);
	shownFrame = static_cast<int>(playback[static_cast<int>(model.Shown(state))].frame);
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
		blood.Draw();
		glPopMatrix();
	};
	if (Alive())
		drawBlood();
	drawBlood(); // even when dead

	model.BindTexture();
	glRotatef(rotA, 0, 1, 0);
	const float figure = Ink::figureScale();
	glScalef(figure, figure, figure);
	model.Show(state, playback);

	glPopMatrix();
	glPopMatrix();
}

void Player::TakeHit(int dmg, bool ignoreArmor) {
	if (Scenario::godMode())
		return;
	const int s = static_cast<int>(scale);
	if (Alive()) {
		stats.LoseHP(stats.HitDamage(dmg, ignoreArmor));
		blood.Splash(s);
	}

	if (!Alive() && state != ModelState::Die) {
		setModelState(ModelState::Die);
		model.dieSound.Play();

		blood.Splash(s);
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
