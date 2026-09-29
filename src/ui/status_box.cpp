#include "status_box.h"
#include "ui_draw.h"
#include "../graphics/font.h"
#include <GL/gl.h>
#include <algorithm>
#include <vector>

using namespace ui;

namespace {
// ---- layout ----
constexpr float TOP = 86.f;		   // top edge of the box
constexpr float LINE_H = 6.f;	   // baseline to baseline
constexpr float PAD_X = 7.f;	   // text to the side frames
constexpr float GLYPH_LOW = 1.8f;  // the status font draws its letters this far above the pen y...
constexpr float GLYPH_HIGH = 4.2f; // ...up to here
constexpr float PAD_Y = 2.8f;	   // letters to the top and bottom frames
constexpr float MIN_WIDTH = 40.f;
constexpr float FADE_IN_MS = 150.f;
constexpr float FADE_OUT_MS = 500.f;

std::vector<std::string> splitLines(const std::string& message) {
	std::vector<std::string> lines;
	for (size_t start = 0; start <= message.size();) {
		size_t end = std::min(message.find('\n', start), message.size());
		lines.push_back(message.substr(start, end - start));
		start = end + 1;
	}
	while (!lines.empty() && lines.back().empty()) // "Now you are level 2\n"
		lines.pop_back();
	return lines;
}
} // namespace

namespace StatusBox {
void draw(const std::string& message, int ageMs, int shownMs, int resX, int resY, Font& font) {
	std::vector<std::string> lines = splitLines(message);
	if (lines.empty())
		return;
	auto age = static_cast<float>(ageMs);
	float alpha = std::clamp(std::min(age / FADE_IN_MS, (static_cast<float>(shownMs) - age) / FADE_OUT_MS), 0.f, 1.f);
	if (alpha <= 0.f)
		return;

	// Font::print resets the modelview, so the canvas is set through the projection.
	float canvasW = 100.f * static_cast<float>(resX) / static_cast<float>(resY);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, canvasW, 0, 100, -21, 21);
	glMatrixMode(GL_MODELVIEW);

	float textW = 0.f;
	for (const std::string& line : lines)
		textW = std::max(textW, font.TextWidth(line.c_str()));
	float w = std::max(MIN_WIDTH, textW + 2 * PAD_X);
	float h = 2 * PAD_Y + GLYPH_HIGH - GLYPH_LOW + LINE_H * static_cast<float>(lines.size() - 1);
	Rect box = {(canvasW - w) / 2, TOP - h, w, h};

	beginShapes();
	fillRect({box.x + 0.7f, box.y - 0.9f, box.w, box.h}, BLACK, BLACK, 0.45f * alpha); // drop shadow
	panel(box, 0.92f * alpha, alpha);

	beginText();
	float y = TOP - PAD_Y - GLYPH_HIGH;
	for (const std::string& line : lines) {
		textCentered(font, box.cx(), y, line.c_str(), GOLD, alpha);
		y -= LINE_H;
	}

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glColor3f(1, 1, 1);
}
} // namespace StatusBox
