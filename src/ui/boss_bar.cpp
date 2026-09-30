#include "boss_bar.h"
#include "ui_draw.h"
#include "../graphics/font.h"
#include <GL/gl.h>
#include <algorithm>

using namespace ui;

namespace {
constexpr float WIDTH = 70.f;
constexpr float BAR_BOTTOM = 5.f;
constexpr float BAR_H = 2.4f;
constexpr float NAME_GAP = 1.6f; // bar top to the name's pen y
constexpr Color BLOOD_TOP = {0.85f, 0.14f, 0.08f};
constexpr Color BLOOD_BOTTOM = {0.45f, 0.05f, 0.03f};
} // namespace

namespace BossBar {
void draw(const char* name, float ratio, int resX, int resY, Font& font) {
	float canvasW = 100.f * static_cast<float>(resX) / static_cast<float>(resY);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, canvasW, 0, 100, -21, 21);
	glMatrixMode(GL_MODELVIEW);

	Rect bar = {(canvasW - WIDTH) / 2, BAR_BOTTOM, WIDTH, BAR_H};
	beginShapes();
	fillRect({bar.x - 0.6f, bar.y - 0.6f, bar.w + 1.2f, bar.h + 1.2f}, BLACK, BLACK, 0.6f);
	fillRect({bar.x, bar.y, bar.w * std::clamp(ratio, 0.f, 1.f), bar.h}, BLOOD_TOP, BLOOD_BOTTOM, 1.f);
	strokeRect(bar, GOLD_DIM, 1.f, 1.5f);
	diamond(bar.x, bar.cy(), 1.2f, GOLD, 1.f);
	diamond(bar.x + bar.w, bar.cy(), 1.2f, GOLD, 1.f);

	beginText();
	float y = bar.y + bar.h + NAME_GAP;
	textCentered(font, bar.cx() + 0.3f, y - 0.3f, name, BLACK);
	textCentered(font, bar.cx(), y, name, GOLD_BRIGHT);

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glColor3f(1, 1, 1);
}
} // namespace BossBar
