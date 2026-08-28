#include "hud.h"
#include <GL/gl.h>

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
} // namespace Hud
