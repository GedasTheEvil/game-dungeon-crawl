#include "monster_draw.h"
#include "../render/gl/gl_includes.h"
#include "../render/gl/matrix_stack.h"
#include <algorithm>
#include <cmath>
#include "../render/ink.h"
#include "../render/lighting.h"
#include "particles_draw.h"
#include "../ui/hud/player_hud.h"
#include "../sim/model/render_config.h"
#include "../level/tuning/gameplay_monster_kinds.h"

namespace {
// Health bar in world units (a tile is 40), the same for every monster.
constexpr float HEALTH_BAR_WIDTH = 14.f;
constexpr float HEALTH_BAR_HEIGHT = 1.5f;
constexpr float HEALTH_BAR_GAP = 3.f; // between the model and the bar
// While poisoned: a lime outline, the fill pulses from venom green to lime, whatever the health.
constexpr Rgb POISON_BAR = {0.6f, 0.95f, 0.15f};
constexpr Rgb POISON_BAR_DARK = {0.08f, 0.4f, 0.1f};
constexpr int POISON_PULSE_MS = 800;
// Its status cards (plan monster-status-icons), left-aligned over the bar: 1.5 times its height (the user's start
// value; tune by play-test), no seconds, no pips (too small to read).
constexpr float STATUS_CARD = 1.5f * HEALTH_BAR_HEIGHT;
constexpr float STATUS_GAP = 0.5f; // bar top to the cards (their frame sticks out), and between two cards

// The row of cards in the bar's billboard space, the bar's left end at x = -w; under a roosting flyer's bar (its
// body is above it).
void drawStatusRow(const Monster& mon, float w, const TextureRegistry& textures) {
	const StatusList statuses = mon.Statuses();
	if (statuses.count == 0)
		return;
	for (int i = 0; i < statuses.count; i++)
		PlayerHud::drawStatusCard(
			statuses.items[static_cast<size_t>(i)],
			{-w + STATUS_GAP / 2 + static_cast<float>(i) * (STATUS_CARD + STATUS_GAP),
			 mon.Roosting() ? -STATUS_GAP - STATUS_CARD : HEALTH_BAR_HEIGHT + STATUS_GAP, // a roost's bar hangs low
			 STATUS_CARD,
			 STATUS_CARD},
			textures.hudIcons.ID(),
			textures.statusCards.ID(),
			false
		);
	glDisable(GL_BLEND);
}

void drawHealthBar(const Monster& mon, const TextureRegistry& textures) {
	const Texture& bar = textures.loadingBar;
	const MonsterType& type = *mon.Type();
	// Above the model's frame 0 top; a roosting flyer's bar hangs under it (the ceiling is above).
	const float drawScale = type.scale * Ink::figureScale();
	float y = type.model.referenceTop * drawScale + HEALTH_BAR_GAP;
	if (mon.Roosting())
		y = type.model.idleBottom * drawScale - HEALTH_BAR_GAP - HEALTH_BAR_HEIGHT;
	gMv.push();
	gMv.translate(0, y, 0);
	// Billboard: keep where the anchor is, drop the camera and model rotation, keep the scene's scale.
	Mat4 mv = gMv.top();
	float s = std::sqrt(mv.m[0] * mv.m[0] + mv.m[1] * mv.m[1] + mv.m[2] * mv.m[2]);
	for (size_t c = 0; c < 3; c++)
		for (size_t r = 0; r < 3; r++)
			mv.m[c * 4 + r] = c == r ? s : 0.f;
	gMv.load(mv);

	Batch& world = Lighting::batch();
	bar.Bind();
	Lighting::setEmissive(true);
	float w = HEALTH_BAR_WIDTH / 2;
	float h = HEALTH_BAR_HEIGHT;
	float o = 0.1f; // outline just outside the bar
	if (mon.Poisoned())
		world.colour(POISON_BAR.r, POISON_BAR.g, POISON_BAR.b, 0.9f);
	else
		world.colour(1, 1, 1, 0.9f);
	world.begin(Primitive::LineLoop);
	world.vertex(-w - o, -o, 0);
	world.vertex(w + o, -o, 0);
	world.vertex(w + o, h + o, 0);
	world.vertex(-w - o, h + o, 0);
	world.end();

	glEnable(GL_BLEND);
	float ratio = mon.MaxHealth() <= 0
					  ? 0.f
					  : std::clamp(static_cast<float>(mon.Health()) / static_cast<float>(mon.MaxHealth()), 0.f, 1.f);
	float right = -w + 2 * w * ratio;
	if (mon.Poisoned()) {
		const float t = 0.5f + 0.5f * std::sin(
										  static_cast<float>(GameClock::now() % POISON_PULSE_MS) /
										  static_cast<float>(POISON_PULSE_MS) * 2.f * static_cast<float>(M_PI)
									  );
		auto mix = [t](float dark, float lit) { return dark + (lit - dark) * t; };
		world.colour(
			mix(POISON_BAR_DARK.r, POISON_BAR.r),
			mix(POISON_BAR_DARK.g, POISON_BAR.g),
			mix(POISON_BAR_DARK.b, POISON_BAR.b)
		);
	} else
		world.colour(3 * (1 - ratio), 3 * ratio, 0);
	world.begin(Primitive::Quads);
	world.texCoord(0, 0);
	world.vertex(-w, 0, 0);
	world.texCoord(1, 0);
	world.vertex(right, 0, 0);
	world.texCoord(1, 1);
	world.vertex(right, h, 0);
	world.texCoord(0, 1);
	world.vertex(-w, h, 0);
	world.end();
	glDisable(GL_BLEND);
	drawStatusRow(mon, w, textures);

	Lighting::setEmissive(false);
	world.colour(1, 1, 1);
	gMv.pop();
}
} // namespace

void DrawMonster(const Monster& mon, const CharacterModel& model, const TextureRegistry& textures, int texture) {
	const bool plain = texture >= 0;
	const MonsterType& type = *mon.Type();
	const float scale = type.scale;
	// A screech's tell shakes the body (plan giant-bat-screech).
	const float shake = mon.Screeching() ? BAT_SHAKE * std::sin(static_cast<float>(GameClock::now()) * 0.11f) : 0.f;
	gMv.push();
	gMv.translate(
		RenderConfig::TILE_SIZE * mon.LocalX() - RenderConfig::TILE_HALF + shake,
		mon.DrawnLift() + shake * 0.5f,
		-30.f - mon.Tomb() + mon.ClimbDepth()
	);
	gMv.push(); // will add rotation

	// Idle monsters keep up the disguise, a hidden one (under the sand) shows none; the boss's bar is on the HUD.
	if (!plain && mon.Alive() && mon.Alerted() && !mon.Hidden() && !type.isBoss())
		drawHealthBar(mon, textures);

	gMv.scale(scale, scale, scale);

	auto drawBlood = [&] {
		gMv.push();
		gMv.scale(0.5f / scale, 0.5f / scale, 0.5f / scale);
		textures.nullTex.Bind();
		Particles::Draw(mon.Blood());
		gMv.pop();
	};
	if (!plain && mon.Alive())
		drawBlood();
	if (!plain)
		drawBlood(); // even when dead
	if (!plain && mon.Alive() && mon.Bleeds() && !mon.Hidden()) {
		textures.nullTex.Bind();
		Particles::DrawBleedDrops(type.model.referenceTop, static_cast<int>(mon.CentreX() * 10.f));
	}

	model.BindTexture();
	// Climbing it faces the ladder, its back to the camera.
	gMv.rotate(type.rotA + (mon.ShowsClimb() ? 180.f : 90.f * static_cast<float>(mon.Facing())), 0, 1, 0);

	const float figure = Ink::figureScale();
	gMv.scale(figure, figure, figure);
	if (const float roll = mon.HoldRoll(); roll != 0.f) { // the death roll: over along its body, round its middle
		const float middle = type.model.referenceTop * 0.5f;
		gMv.translate(0, middle, 0);
		gMv.rotate(roll * static_cast<float>(mon.Facing()), 0, 0, 1);
		gMv.translate(0, -middle, 0);
	}
	model.Show(mon.ShownState(), mon.Playback(), texture);

	gMv.pop();
	gMv.pop();
}

void DrawEntombed(const MonsterType& type, const CharacterModel& model) {
	gMv.push();
	gMv.translate(-RenderConfig::TILE_HALF, 0, -30.f - MUMMY_COFFIN_DEPTH);
	gMv.scale(type.scale, type.scale, type.scale);
	model.BindTexture();
	gMv.rotate(type.rotA, 0, 1, 0);
	const float figure = Ink::figureScale();
	gMv.scale(figure, figure, figure);
	model.Show(ModelState::Idle, ClipPlayback{});
	gMv.pop();
}
