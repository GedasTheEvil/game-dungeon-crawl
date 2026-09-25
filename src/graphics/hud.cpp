#include "hud.h"
#include <GL/gl.h>
#include <cmath>
#include "../world/level.h"

namespace {
float clampRatio(float value) {
	if (value < 0.0f)
		return 0.0f;
	if (value > 1.0f)
		return 1.0f;
	return value;
}
} // namespace

namespace Hud {
void drawBar(float left, float bottom, float width, float height, float ratio, float red, float green, float blue) {
	glColor3f(1, 1, 1);
	glBegin(GL_LINE_LOOP);
	glVertex3f(left, bottom, 0);
	glVertex3f(left + width, bottom, 0);
	glVertex3f(left + width, bottom + height, 0);
	glVertex3f(left, bottom + height, 0);
	glEnd();

	glColor3f(red, green, blue);
	glBegin(GL_QUADS);
	glVertex3f(left, bottom, 0);
	glVertex3f(left + width * ratio, bottom, 0);
	glVertex3f(left + width * ratio, bottom + height, 0);
	glVertex3f(left, bottom + height, 0);
	glEnd();
}

void drawPlayerBars(float healthRatio, float staminaRatio) {
	healthRatio = clampRatio(healthRatio);
	staminaRatio = clampRatio(staminaRatio);

	const float barLeft = 3.5f;
	const float barWidth = 20.0f;
	const float barHeight = 2.5f;
	const float hpBottom = 3.5f;
	const float staminaBottom = hpBottom + 3.2f;

	drawBar(barLeft, hpBottom, barWidth, barHeight, healthRatio, 0, 1, 0);
	drawBar(barLeft, staminaBottom, barWidth, barHeight / 2, staminaRatio, 1, 1, 0);
}

void drawKeys(int keysHeld) {
	// Gem colours of the lock colours, in LOCK_COLOUR_NAMES order.
	constexpr float COLOURS[LOCK_COLOUR_COUNT][3] = {
		{0.85f, 0.2f, 0.12f}, {0.2f, 0.35f, 0.95f}, {0.15f, 0.8f, 0.6f}, {1.f, 0.78f, 0.2f}};
	constexpr int SEGMENTS = 12;
	float left = 26.5f;
	float bottom = 4.2f;
	for (int c = 0; c < LOCK_COLOUR_COUNT; c++) {
		if ((keysHeld & (1 << c)) == 0)
			continue;
		glColor3f(COLOURS[c][0], COLOURS[c][1], COLOURS[c][2]);
		// Ring (bow) on the left, shaft to the right, two teeth down.
		float cx = left + 1.f;
		float cy = bottom + 1.f;
		glBegin(GL_TRIANGLE_FAN);
		glVertex3f(cx, cy, 0);
		for (int s = 0; s <= SEGMENTS; s++) {
			float a = 6.2831853f * static_cast<float>(s) / SEGMENTS;
			glVertex3f(cx + std::cos(a), cy + 1.2f * std::sin(a), 0);
		}
		glEnd();
		glBegin(GL_QUADS);
		glVertex3f(cx + 0.8f, cy - 0.25f, 0);
		glVertex3f(cx + 3.6f, cy - 0.25f, 0);
		glVertex3f(cx + 3.6f, cy + 0.25f, 0);
		glVertex3f(cx + 0.8f, cy + 0.25f, 0);
		for (float tooth : {2.6f, 3.3f}) {
			glVertex3f(cx + tooth, cy - 1.f, 0);
			glVertex3f(cx + tooth + 0.3f, cy - 1.f, 0);
			glVertex3f(cx + tooth + 0.3f, cy, 0);
			glVertex3f(cx + tooth, cy, 0);
		}
		glEnd();
		left += 5.f;
	}
	glColor3f(1, 1, 1);
}
} // namespace Hud
