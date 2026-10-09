#include "ui_layout.h"
#include <algorithm>
#include <cmath>

namespace ui {

namespace {
// The screen tabs, at the top right of the canvas.
constexpr float TAB_W = 9.5f;
constexpr float TAB_H = 7.f;
constexpr float TAB_GAP = 1.f;
constexpr float TABS_RIGHT = 158.f;
constexpr float TABS_Y = 88.f; // centred on the title rule
} // namespace

Rect visibleArea(float canvasW, float canvasH, int resX, int resY) {
	float aspect = static_cast<float>(resX) / static_cast<float>(resY);
	if (aspect >= canvasW / canvasH) {
		float w = canvasH * aspect;
		return {(canvasW - w) / 2, 0, w, canvasH};
	}
	float h = canvasW / aspect;
	return {0, (canvasH - h) / 2, canvasW, h};
}

void toCanvas(const Rect& area, int resX, int resY, int mouseX, int mouseY, float& x, float& y) {
	x = area.x + area.w * static_cast<float>(mouseX) / static_cast<float>(resX);
	y = area.y + area.h - area.h * static_cast<float>(mouseY) / static_cast<float>(resY);
}

void toWindow(const Rect& area, int resX, int resY, float x, float y, int& mouseX, int& mouseY) {
	mouseX = static_cast<int>(std::lround((x - area.x) / area.w * static_cast<float>(resX)));
	mouseY = static_cast<int>(std::lround((area.y + area.h - y) / area.h * static_cast<float>(resY)));
}

Rect screenTabRect(int tab) {
	float x0 = TABS_RIGHT - SCREEN_TAB_COUNT * TAB_W - (SCREEN_TAB_COUNT - 1) * TAB_GAP;
	return {x0 + static_cast<float>(tab) * (TAB_W + TAB_GAP), TABS_Y, TAB_W, TAB_H};
}

int screenTabAt(float x, float y) {
	for (int tab = 0; tab < SCREEN_TAB_COUNT; tab++)
		if (screenTabRect(tab).contains(x, y))
			return tab;
	return -1;
}

} // namespace ui

namespace StatusBox {
namespace {
constexpr float TOP = 86.f;		   // top edge of the box
constexpr float LINE_H = 6.f;	   // baseline to baseline
constexpr float PAD_X = 7.f;	   // text to the side frames
constexpr float GLYPH_LOW = 1.8f;  // the status font draws its letters this far above the pen y...
constexpr float GLYPH_HIGH = 4.2f; // ...up to here
constexpr float PAD_Y = 2.8f;	   // letters to the top and bottom frames
constexpr float MIN_WIDTH = 40.f;
constexpr float FADE_IN_MS = 150.f;
constexpr float FADE_OUT_MS = 500.f;
} // namespace

std::vector<std::string> SplitLines(const std::string& message) {
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

float Alpha(int ageMs, int shownMs) {
	auto age = static_cast<float>(ageMs);
	return std::clamp(std::min(age / FADE_IN_MS, (static_cast<float>(shownMs) - age) / FADE_OUT_MS), 0.f, 1.f);
}

ui::Rect Box(float canvasW, float textW, int lines) {
	float w = std::max(MIN_WIDTH, textW + 2 * PAD_X);
	float h = 2 * PAD_Y + GLYPH_HIGH - GLYPH_LOW + LINE_H * static_cast<float>(lines - 1);
	return {(canvasW - w) / 2, TOP - h, w, h};
}

float LineY(int i) {
	float y = TOP - PAD_Y - GLYPH_HIGH;
	for (int k = 0; k < i; k++)
		y -= LINE_H;
	return y;
}
} // namespace StatusBox

int LevelGem::GemOf(int level) { return std::clamp((level - 1) / LEVELS_PER_GEM, 0, GEM_COUNT - 1); }
