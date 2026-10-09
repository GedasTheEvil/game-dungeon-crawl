#include "status_box.h"
#include "ui_draw.h"
#include "../graphics/font.h"
#include <GL/gl.h>
#include <algorithm>
#include <vector>

using namespace ui;

namespace StatusBox {
void draw(const std::string& message, int ageMs, int shownMs, int resX, int resY, Font& font) {
	std::vector<std::string> lines = SplitLines(message);
	if (lines.empty())
		return;
	const float alpha = Alpha(ageMs, shownMs);
	if (alpha <= 0.f)
		return;

	// Font::print resets the modelview, so the canvas is set through the projection.
	float canvasW = beginSquareCanvas(CANVAS_H, resX, resY);

	float textW = 0.f;
	for (const std::string& line : lines)
		textW = std::max(textW, font.TextWidth(line.c_str()));
	const Rect box = Box(canvasW, textW, static_cast<int>(lines.size()));

	beginShapes();
	fillRect({box.x + 0.7f, box.y - 0.9f, box.w, box.h}, BLACK, BLACK, 0.45f * alpha); // drop shadow
	panel(box, 0.92f * alpha, alpha);

	beginText();
	for (size_t i = 0; i < lines.size(); i++)
		textCentered(font, box.cx(), LineY(static_cast<int>(i)), lines[i].c_str(), GOLD, alpha);

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glColor3f(1, 1, 1);
}
} // namespace StatusBox
