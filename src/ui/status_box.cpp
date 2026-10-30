#include "status_box.h"
#include "ui_draw.h"
#include "../graphics/font.h"
#include <GL/gl.h>
#include <algorithm>
#include <vector>

using namespace ui;

namespace StatusBox {
namespace {
constexpr Color GOOD = {0.55f, 0.88f, 0.72f}; // faience green: a level up, heka taken
constexpr Color DANGER = {1.f, 0.48f, 0.34f}; // a curse, poison

Color toneColour(StatusTone tone) {
	switch (tone) {
	case StatusTone::Good:
		return GOOD;
	case StatusTone::Danger:
		return DANGER;
	case StatusTone::Plain:
		break;
	}
	return GOLD;
}
} // namespace

void draw(const std::vector<const StatusMessage*>& shown, int now, int resX, int resY, Font& font) {
	if (shown.empty())
		return;
	// Font::print resets the modelview, so the canvas is set through the projection.
	float canvasW = beginSquareCanvas(CANVAS_H, resX, resY);
	float top = TOP;
	for (const StatusMessage* m : shown) {
		std::vector<std::string> lines = SplitLines(m->Shown());
		if (lines.empty())
			continue;
		const float alpha = FadeAlpha(now - m->bornMs, m->EndMs() - now);
		float textW = 0.f;
		for (const std::string& line : lines)
			textW = std::max(textW, font.TextWidth(line.c_str()));
		const Rect box = Box(canvasW, textW, static_cast<int>(lines.size()), top);
		top = box.y - GAP;
		if (alpha <= 0.f)
			continue;

		beginShapes();
		fillRect({box.x + 0.7f, box.y - 0.9f, box.w, box.h}, BLACK, BLACK, 0.45f * alpha); // drop shadow
		panel(box, 0.92f * alpha, alpha);

		beginText();
		const Color colour = toneColour(m->tone);
		for (size_t i = 0; i < lines.size(); i++)
			textCentered(font, box.cx(), LineY(static_cast<int>(i), box.y + box.h), lines[i].c_str(), colour, alpha);
	}

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glColor3f(1, 1, 1);
}
} // namespace StatusBox
