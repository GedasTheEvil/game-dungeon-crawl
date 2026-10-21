#include "zone_banner.h"
#include "ui_draw.h"
#include "../graphics/font.h"
#include <GL/gl.h>
#include <algorithm>

using namespace ui;

namespace {
constexpr float TOP = 80.f; // the panel's top, under the status box
constexpr float PAD_X = 6.f;
constexpr float PAD_Y = 3.f;
constexpr float TITLE_H = 7.f; // a line of the title font, of the body font
constexpr float BODY_H = 4.6f;
} // namespace

namespace ZoneBanner {
float Alpha(int ageMs, bool zone) {
	const int shownMs = zone ? ZONE_MS : LEVEL_MS;
	if (ageMs < 0 || ageMs >= shownMs)
		return 0.f;
	const float in = static_cast<float>(ageMs) / static_cast<float>(FADE_MS);
	const float out = static_cast<float>(shownMs - ageMs) / static_cast<float>(FADE_MS);
	return std::clamp(std::min(in, out), 0.f, 1.f);
}

void draw(const LevelBanner& banner, float alpha, int resX, int resY, Font& title, Font& body) {
	if (alpha <= 0.f)
		return;
	const float canvasW = beginSquareCanvas(100.f, resX, resY);
	Font& head = banner.zone ? title : body;
	const float headH = banner.zone ? TITLE_H : BODY_H;
	float textW = head.TextWidth(banner.title.c_str());
	textW = std::max(textW, body.TextWidth(banner.subtitle.c_str()));
	textW = std::max(textW, body.TextWidth(banner.guardian.c_str()));
	const int bodyLines = (banner.subtitle.empty() ? 0 : 1) + (banner.guardian.empty() ? 0 : 1);
	const float h = headH + static_cast<float>(bodyLines) * BODY_H + 2.f * PAD_Y;
	const Rect box = {(canvasW - textW) / 2.f - PAD_X, TOP - h, textW + 2.f * PAD_X, h};

	beginShapes();
	fillRect({box.x + 0.7f, box.y - 0.9f, box.w, box.h}, BLACK, BLACK, 0.45f * alpha); // drop shadow
	panel(box, 0.92f * alpha, alpha);
	if (banner.zone) { // the gate: a diamond at each end of the title
		diamond(box.x, box.y + box.h - PAD_Y - headH * 0.4f, 1.4f, GOLD, alpha);
		diamond(box.x + box.w, box.y + box.h - PAD_Y - headH * 0.4f, 1.4f, GOLD, alpha);
	}

	beginText();
	float y = box.y + box.h - PAD_Y - headH * 0.75f;
	textCentered(head, box.cx() + 0.3f, y - 0.3f, banner.title.c_str(), BLACK, alpha);
	textCentered(head, box.cx(), y, banner.title.c_str(), banner.zone ? GOLD_BRIGHT : GOLD, alpha);
	y -= (headH + BODY_H) / 2.f;
	if (!banner.subtitle.empty()) {
		textCentered(body, box.cx(), y, banner.subtitle.c_str(), GOLD, alpha);
		y -= BODY_H;
	}
	if (!banner.guardian.empty())
		textCentered(body, box.cx(), y, banner.guardian.c_str(), GOLD_DIM, alpha);

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glColor3f(1, 1, 1);
}
} // namespace ZoneBanner
