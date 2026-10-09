#include "player_view.h"
#include <GL/gl.h>
#include <cmath>
#include "../graphics/ink.h"
#include "../graphics/particles.h"

namespace {
constexpr float DEG_TO_RAD = 3.14159265f / 180.f;
} // namespace

bool PlayerView::Load(const char* name, Texture&& texture, Player& player) {
	if (!model.Load(name, std::move(texture), PLAYER_CLIPS, true)) // Fist() reads any frame
		return false;
	player.SetModel(model.Info());
	findFists();
	return true;
}

// The fists are raised in front of the chest in every clip: the most forward corners (+z) at 50-85 % of the
// idle height, one on each side.
void PlayerView::findFists() {
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

std::array<float, 3> PlayerView::Fist(const Player& player, int dir) const {
	const float s = player.scale * Ink::figureScale();
	const int fist = fists[dir > 0 ? 0 : 1]; // turned right, the model's -x side is towards the camera
	const AnimatedModel* clip = model.Clip(model.Info().Shown(player.State()));
	std::array<float, 3> v{0, 0.75f, 0.2f}; // no fists found: in front of the chest (the model is 1 tall)
	if (fist >= 0 && fist < clip->VertexCount() && player.ShownFrame() < clip->FrameCount())
		v = clip->Vertex(player.ShownFrame(), fist);
	// As Draw(): moved back, scaled, turned rotA round y.
	const float a = player.rotA * DEG_TO_RAD;
	return {s * (v[0] * std::cos(a) + v[2] * std::sin(a)), s * v[1],
			-30.f + player.depthOffset + s * (-v[0] * std::sin(a) + v[2] * std::cos(a))};
}

void PlayerView::Draw(const Player& player, const TextureRegistry& textures) const {
	const float scale = player.scale;
	glPushMatrix();
	glTranslatef(0, 0, -30 + player.depthOffset);
	glPushMatrix(); // will add rotation
	glScalef(scale, scale, scale);

	auto drawBlood = [&] {
		glPushMatrix();
		glScalef(0.5f / scale, 0.5f / scale, 0.5f / scale);
		textures.nullTex.Bind();
		Particles::Draw(player.Blood());
		glPopMatrix();
	};
	if (player.Alive())
		drawBlood();
	drawBlood(); // even when dead

	model.BindTexture();
	glRotatef(player.rotA, 0, 1, 0);
	const float figure = Ink::figureScale();
	glScalef(figure, figure, figure);
	model.Show(player.State(), player.Playback());

	glPopMatrix();
	glPopMatrix();
}
