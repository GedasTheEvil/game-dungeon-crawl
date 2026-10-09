#include "monster_draw.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include "../graphics/ink.h"
#include "../graphics/lighting.h"
#include "../graphics/particles.h"
#include "../graphics/render_config.h"

namespace {
// Health bar in world units (a tile is 40), the same for every monster.
constexpr float HEALTH_BAR_WIDTH = 14.f;
constexpr float HEALTH_BAR_HEIGHT = 1.5f;
constexpr float HEALTH_BAR_GAP = 3.f; // between the model and the bar
// While poisoned: a lime outline, the fill pulses from venom green to lime, whatever the health.
constexpr Rgb POISON_BAR = {0.6f, 0.95f, 0.15f};
constexpr Rgb POISON_BAR_DARK = {0.08f, 0.4f, 0.1f};
constexpr int POISON_PULSE_MS = 800;

void drawHealthBar(const Monster& mon, const Texture& bar) {
	const MonsterType& type = *mon.Type();
	// Above the model's frame 0 top; a roosting flyer's bar hangs under it (the ceiling is above).
	const float drawScale = type.scale * Ink::figureScale();
	float y = type.model.referenceTop * drawScale + HEALTH_BAR_GAP;
	if (mon.Roosting())
		y = type.model.idleBottom * drawScale - HEALTH_BAR_GAP - HEALTH_BAR_HEIGHT;
	glPushMatrix();
	glTranslatef(0, y, 0);
	// Billboard: keep where the anchor is, drop the camera and model rotation, keep the scene's scale.
	GLfloat mv[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, mv);
	float s = std::sqrt(mv[0] * mv[0] + mv[1] * mv[1] + mv[2] * mv[2]);
	for (int c = 0; c < 3; c++)
		for (int r = 0; r < 3; r++)
			mv[c * 4 + r] = c == r ? s : 0.f;
	glLoadMatrixf(mv);

	bar.Bind();
	Lighting::setEmissive(true);
	float w = HEALTH_BAR_WIDTH / 2;
	float h = HEALTH_BAR_HEIGHT;
	float o = 0.1f; // outline just outside the bar
	if (mon.Poisoned())
		glColor4f(POISON_BAR.r, POISON_BAR.g, POISON_BAR.b, 0.9);
	else
		glColor4f(1, 1, 1, 0.9);
	glBegin(GL_LINE_LOOP);
	glVertex3f(-w - o, -o, 0);
	glVertex3f(w + o, -o, 0);
	glVertex3f(w + o, h + o, 0);
	glVertex3f(-w - o, h + o, 0);
	glEnd();

	glEnable(GL_BLEND);
	float ratio = type.maxHealth <= 0
					  ? 0.f
					  : std::clamp(static_cast<float>(mon.Health()) / static_cast<float>(type.maxHealth), 0.f, 1.f);
	float right = -w + 2 * w * ratio;
	if (mon.Poisoned()) {
		const float t = 0.5f + 0.5f * std::sin(static_cast<float>(GameClock::now() % POISON_PULSE_MS) /
											   static_cast<float>(POISON_PULSE_MS) * 2.f * static_cast<float>(M_PI));
		auto mix = [t](float dark, float lit) { return dark + (lit - dark) * t; };
		glColor3f(mix(POISON_BAR_DARK.r, POISON_BAR.r), mix(POISON_BAR_DARK.g, POISON_BAR.g),
				  mix(POISON_BAR_DARK.b, POISON_BAR.b));
	} else
		glColor3f(3 * (1 - ratio), 3 * ratio, 0);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(-w, 0, 0);
	glTexCoord2f(1, 0);
	glVertex3f(right, 0, 0);
	glTexCoord2f(1, 1);
	glVertex3f(right, h, 0);
	glTexCoord2f(0, 1);
	glVertex3f(-w, h, 0);
	glEnd();
	glDisable(GL_BLEND);

	Lighting::setEmissive(false);
	glColor3f(1, 1, 1);
	glPopMatrix();
}
} // namespace

void DrawMonster(const Monster& mon, const CharacterModel& model, const TextureRegistry& textures) {
	const MonsterType& type = *mon.Type();
	const float scale = type.scale;
	glPushMatrix();
	glTranslatef(RenderConfig::TILE_SIZE * mon.LocalX() - RenderConfig::TILE_HALF, mon.DrawnLift(), -30.f - mon.Tomb());
	glPushMatrix(); // will add rotation

	if (mon.Alive() && mon.Alerted() && !type.isBoss()) // idle monsters keep up the disguise; the boss's bar is on the HUD
		drawHealthBar(mon, textures.loadingBar);

	glScalef(scale, scale, scale);

	auto drawBlood = [&] {
		glPushMatrix();
		glScalef(0.5f / scale, 0.5f / scale, 0.5f / scale);
		textures.nullTex.Bind();
		Particles::Draw(mon.Blood());
		glPopMatrix();
	};
	if (mon.Alive())
		drawBlood();
	drawBlood(); // even when dead

	model.BindTexture();
	glRotatef(type.rotA + 90.f * static_cast<float>(mon.Facing()), 0, 1, 0);

	const float figure = Ink::figureScale();
	glScalef(figure, figure, figure);
	model.Show(mon.State(), mon.Playback());

	glPopMatrix();
	glPopMatrix();
}
