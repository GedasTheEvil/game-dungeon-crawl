#include "ui_draw.h"
#include "batch.h"
#include "font.h"
#include "gl_includes.h"
#include "gl_legacy.h"
#include "matrix_stack.h"
#include <algorithm>
#include <cmath>
#include <utility>
#include "../core/paths.h"

namespace ui {

namespace {
// Every part here draws through this one batch (plan gl-core-ui): flat shapes untextured, pictures textured.
Batch& batch(DefaultShader::Kind kind) {
	static Batch b;
	b.style.kind = kind;
	return b;
}

Batch& shapes() { return batch(DefaultShader::Kind::Colour); }
} // namespace

void fillRect(const Rect& r, Color top, Color bottom, float alpha) {
	Batch& b = shapes();
	b.begin(Primitive::Quads);
	b.colour(bottom.r, bottom.g, bottom.b, alpha);
	b.vertex(r.x, r.y);
	b.vertex(r.x + r.w, r.y);
	b.colour(top.r, top.g, top.b, alpha);
	b.vertex(r.x + r.w, r.y + r.h);
	b.vertex(r.x, r.y + r.h);
	b.end();
}

void strokeRect(const Rect& r, Color c, float alpha, float width) {
	Batch& b = shapes();
	glLineWidth(width);
	b.colour(c.r, c.g, c.b, alpha);
	b.begin(Primitive::LineLoop);
	b.vertex(r.x, r.y);
	b.vertex(r.x + r.w, r.y);
	b.vertex(r.x + r.w, r.y + r.h);
	b.vertex(r.x, r.y + r.h);
	b.end();
	glLineWidth(1);
}

void line(float x0, float y0, float x1, float y1, Color c, float alpha, float width) {
	Batch& b = shapes();
	glLineWidth(width);
	b.colour(c.r, c.g, c.b, alpha);
	b.begin(Primitive::Lines);
	b.vertex(x0, y0);
	b.vertex(x1, y1);
	b.end();
	glLineWidth(1);
}

void diamond(float x, float y, float size, Color c, float alpha) {
	Batch& b = shapes();
	b.colour(c.r, c.g, c.b, alpha);
	b.begin(Primitive::Quads);
	b.vertex(x - size, y);
	b.vertex(x, y - size);
	b.vertex(x + size, y);
	b.vertex(x, y + size);
	b.end();
}

void triangle(float x0, float y0, float x1, float y1, float x2, float y2, Color c, float alpha) {
	Batch& b = shapes();
	b.colour(c.r, c.g, c.b, alpha);
	b.begin(Primitive::Triangles);
	b.vertex(x0, y0);
	b.vertex(x1, y1);
	b.vertex(x2, y2);
	b.end();
}

void ring(const Rect& inner, float grow, Color c, float alphaIn, float alphaOut) {
	Batch& b = shapes();
	b.begin(Primitive::QuadStrip);
	for (int corner = 0; corner <= 4; corner++) { // counter-clockwise from bottom left, back to the start
		float sx = corner == 1 || corner == 2 ? 1.f : 0.f;
		float sy = corner == 2 || corner == 3 ? 1.f : 0.f;
		b.colour(c.r, c.g, c.b, alphaIn);
		b.vertex(inner.x + sx * inner.w, inner.y + sy * inner.h);
		b.colour(c.r, c.g, c.b, alphaOut);
		b.vertex(inner.x + sx * inner.w + (2 * sx - 1) * grow, inner.y + sy * inner.h + (2 * sy - 1) * grow);
	}
	b.end();
}

void newMark(float x, float y, float radius) {
	ellipse(x, y, radius, radius, BLACK, 0.8f);
	ellipse(x, y, radius * 0.72f, radius * 0.72f, NEW_RED, 1.f);
	ellipse(x - radius * 0.2f, y + radius * 0.2f, radius * 0.25f, radius * 0.25f, {1.f, 0.7f, 0.6f}, 0.8f);
}

void ellipse(float x, float y, float rx, float ry, Color c, float alpha) {
	constexpr int SEGMENTS = 32;
	Batch& b = shapes();
	b.begin(Primitive::TriangleFan);
	b.colour(c.r, c.g, c.b, alpha);
	b.vertex(x, y);
	b.colour(c.r, c.g, c.b, 0.f);
	for (int i = 0; i <= SEGMENTS; i++) {
		float a = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / SEGMENTS;
		b.vertex(x + rx * std::cos(a), y + ry * std::sin(a));
	}
	b.end();
}

void cornerStuds(const Rect& r, float alpha) {
	diamond(r.x, r.y, 1.2f, GOLD, alpha);
	diamond(r.x + r.w, r.y, 1.2f, GOLD, alpha);
	diamond(r.x + r.w, r.y + r.h, 1.2f, GOLD, alpha);
	diamond(r.x, r.y + r.h, 1.2f, GOLD, alpha);
}

void panel(const Rect& r, float alpha, float frameAlpha) {
	fillRect(r, PANEL_TOP, PANEL_BOTTOM, alpha);
	strokeRect(r, BRONZE, frameAlpha, 3.f);
	strokeRect(r.inset(1.1f), GOLD_DIM, 0.8f * frameAlpha, 1.f);
	cornerStuds(r, frameAlpha);
}

float beginSquareCanvas(float height, int resX, int resY, float depth) {
	const float width = height * static_cast<float>(resX) / static_cast<float>(resY);
	beginOrtho({0.f, 0.f, width, height}, depth);
	return width;
}

void beginCanvas(const Rect& area) { beginOrtho(area, 200.f); }

void beginOrtho(const Rect& area, float depth) {
	gProj.load(Mat4::ortho(area.x, area.x + area.w, area.y, area.y + area.h, -depth, depth));
	gMv.loadIdentity();
	GlLegacy::syncMatrices(); // the not yet ported code drawn after a UI part finds the canvas set (plan gl-core-ui)
}

void endScreen() {
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	GlLegacy::texturing(true);
	GlLegacy::resetColour();
}

void loadScreenFonts(Font& title, Font& heading, Font& body, Font& small, float titleSize) {
	title.Load(MEDIA_DIR "textures/fonts/parchment.png", titleSize, 0.3f, true);
	heading.Load(MEDIA_DIR "textures/fonts/parchment.png", 5.f, 0.16f, true);
	body.Load(MEDIA_DIR "textures/fonts/parchment.png", 3.6f, 0.1f, true);
	small.Load(MEDIA_DIR "textures/fonts/parchment.png", 3.f, 0.08f, true);
}

void backdrop(const Rect& area, int textureId) {
	glDisable(GL_BLEND);
	texturedRect(area, textureId, {0.34f, 0.27f, 0.20f});
	beginShapes();
	constexpr float VIGNETTE = 22.f;
	ring(area.inset(VIGNETTE), VIGNETTE, BLACK, 0.f, 0.85f);
}

void titleBar(Font& font, float cx, const char* caption, float reach) {
	constexpr float RULE_Y = 91.5f;
	constexpr float BASELINE = 88.f;
	float titleHalf = font.TextWidth(caption) / 2 + 4;
	line(cx - reach, RULE_Y, cx - titleHalf, RULE_Y, GOLD_DIM, 1.f, 2.f);
	line(cx + titleHalf, RULE_Y, cx + reach, RULE_Y, GOLD_DIM, 1.f, 2.f);
	diamond(cx - reach, RULE_Y, 1.1f, GOLD, 1.f);
	diamond(cx + reach, RULE_Y, 1.1f, GOLD, 1.f);
	diamond(cx - titleHalf + 1.5f, RULE_Y, 0.7f, GOLD, 1.f);
	diamond(cx + titleHalf - 1.5f, RULE_Y, 0.7f, GOLD, 1.f);
	beginText();
	textCentered(font, cx, BASELINE, caption, GOLD);
	beginShapes();
}

// ---- tooltip ----

namespace {
struct TooltipRequest {
	Rect anchor{};
	std::string text;
};
TooltipRequest tooltipWanted;
TooltipTimer tooltipTimer;
} // namespace

void tooltip(const Rect& anchor, const std::string& text) { tooltipWanted = {anchor, text}; }

void hideTooltip() { tooltipTimer.Hide(); }

void drawTooltip(Font& small, int now) {
	const TooltipRequest wanted = std::exchange(tooltipWanted, {});
	if (!tooltipTimer.Want(wanted.anchor, wanted.text, now))
		return;
	const std::vector<std::string> lines = StatusBox::SplitLines(wanted.text);
	float textW = 0.f;
	for (const std::string& line : lines)
		textW = std::max(textW, small.TextWidth(line.c_str()));
	const Rect r = tooltipRect(wanted.anchor, tooltipW(textW), tooltipH(static_cast<int>(lines.size())));
	beginShapes();
	fillRect({r.x + 0.5f, r.y - 0.7f, r.w, r.h}, BLACK, BLACK, 0.4f);
	fillRect(r, PANEL_TOP, PANEL_BOTTOM, 0.95f);
	strokeRect(r, BRONZE, 1.f, 1.5f);
	beginText();
	const float top = r.y + r.h - 0.6f;
	for (size_t i = 0; i < lines.size(); i++)
		textMiddle(
			small,
			r.x + TOOLTIP_PAD,
			top - (static_cast<float>(i) + 0.5f) * TOOLTIP_LINE,
			lines[i].c_str(),
			GOLD
		);
	beginShapes();
}

// ---- screen tabs ----

namespace {
constexpr const char* TAB_NAMES[SCREEN_TAB_COUNT] = {"Inventory", "Draft map", "Journal", "Heka (skills)"};
constexpr const char* TAB_KEYS[SCREEN_TAB_COUNT] = {"I", "M", "J", "K"};

void bar(float x0, float y0, float x1, float y1, Color c) { fillRect({x0, y0, x1 - x0, y1 - y0}, c, c, 1.f); }

} // namespace

void tabIcon(ScreenTab tab, float cx, float cy, float s, Color c) {
	switch (tab) {
	case ScreenTab::Inventory: // a chest: lid, body and the lock between them
		bar(cx - 0.9f * s, cy + 0.2f * s, cx + 0.9f * s, cy + 0.75f * s, c);
		bar(cx - 0.9f * s, cy - 0.8f * s, cx + 0.9f * s, cy + 0.02f * s, c);
		bar(cx - 0.18f * s, cy - 0.3f * s, cx + 0.18f * s, cy + 0.4f * s, c);
		break;
	case ScreenTab::Map: // a sheet with rolled ends
		bar(cx - 0.6f * s, cy - 0.7f * s, cx + 0.6f * s, cy + 0.7f * s, c);
		bar(cx - 0.85f * s, cy + 0.55f * s, cx + 0.85f * s, cy + 0.9f * s, c);
		bar(cx - 0.85f * s, cy - 0.9f * s, cx + 0.85f * s, cy - 0.55f * s, c);
		break;
	case ScreenTab::Journal: // an open book
		bar(cx - 0.95f * s, cy - 0.6f * s, cx - 0.1f * s, cy + 0.75f * s, c);
		bar(cx + 0.1f * s, cy - 0.6f * s, cx + 0.95f * s, cy + 0.75f * s, c);
		bar(cx + 0.45f * s, cy - 0.95f * s, cx + 0.62f * s, cy - 0.6f * s, c);
		break;
	case ScreenTab::Skills: // the ka: two arms raised from a bar
		bar(cx - 0.9f * s, cy - 0.9f * s, cx + 0.9f * s, cy - 0.6f * s, c);
		bar(cx - 0.9f * s, cy - 0.6f * s, cx - 0.6f * s, cy + 0.9f * s, c);
		bar(cx + 0.6f * s, cy - 0.6f * s, cx + 0.9f * s, cy + 0.9f * s, c);
		break;
	}
}

void screenTabs(Font& small, ScreenTab open, int hovered, int held, bool journalNew) {
	beginShapes();
	for (int tab = 0; tab < SCREEN_TAB_COUNT; tab++) {
		bool active = static_cast<int>(open) == tab;
		bool isHovered = hovered == tab && !active;
		Rect r =
			tile(screenTabRect(tab), active ? TileStyle::Lapis : TileStyle::Stone, isHovered, isHovered && held == tab);
		Color c = isHovered ? GOLD_BRIGHT : GOLD;
		tabIcon(static_cast<ScreenTab>(tab), r.x + 3.3f, r.cy(), 1.8f, c);
		beginText();
		textMiddle(small, r.x + 6.4f, r.cy(), TAB_KEYS[tab], isHovered ? TEXT_HOVER : GOLD);
		beginShapes();
		if (journalNew && static_cast<ScreenTab>(tab) == ScreenTab::Journal)
			newMark(r.x + r.w - 1.f, r.y + r.h - 1.f, 1.3f);
	}
	if (hovered >= 0)
		tooltip(screenTabRect(hovered), TAB_NAMES[hovered]);
}

Rect tile(Rect r, TileStyle style, bool hovered, bool held) {
	if (style == TileStyle::Disabled) {
		fillRect(r, {0.10f, 0.08f, 0.06f}, {0.07f, 0.055f, 0.04f}, 0.9f);
		strokeRect(r, BRONZE, 0.6f, 1.f);
		return r;
	}
	if (style == TileStyle::PapyrusDisabled) {
		fillRect(r, {0.62f, 0.52f, 0.38f}, {0.55f, 0.45f, 0.32f}, 0.6f);
		strokeRect(r, INK_FADED, 0.9f, 1.5f);
		return r;
	}

	fillRect({r.x + 0.5f, r.y - 0.7f, r.w, r.h}, BLACK, BLACK, held ? 0.f : 0.4f);
	if (held)
		r.y -= TILE_SINK;
	if (hovered) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		ring(r, 2.2f, GOLD, 0.4f, 0.f);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	bool lapis = style == TileStyle::Lapis;
	Color top = lapis ? LAPIS : STONE_TOP;
	Color bottom = lapis ? LAPIS_DARK : STONE_BOTTOM;
	if (held) {
		top = bottom;
		bottom = lapis ? LAPIS_HELD_BOTTOM : STONE_HELD_BOTTOM;
	} else if (hovered) {
		top = lapis ? LAPIS_HOVER_TOP : STONE_HOVER_TOP;
		bottom = lapis ? LAPIS_HOVER_BOTTOM : STONE_HOVER_BOTTOM;
	}
	fillRect(r, top, bottom, 1.f);
	if (!held) // top highlight
		fillRect({r.x, r.y + r.h - 1.f, r.w, 1.f}, {1, 1, 1}, {1, 1, 1}, hovered ? 0.16f : 0.08f);

	tileFrame(r, style, hovered);
	return r;
}

void tileFrame(const Rect& r, TileStyle style, bool hovered) {
	bool lapis = style == TileStyle::Lapis;
	if (hovered)
		strokeRect(r, GOLD_BRIGHT, 1.f, 2.5f);
	else
		strokeRect(r, lapis ? GOLD : BRONZE, 1.f, lapis ? 2.5f : 1.5f);
	if (lapis || hovered)
		strokeRect(r.inset(0.8f), GOLD_DIM, 0.6f, 1.f);
}

Rect iconWell(const Rect& tile, float inset) {
	float size = tile.h - 2 * inset;
	return {tile.x + inset, tile.y + inset, size, size};
}

void atlasIcon(const Rect& r, int textureId, int cell, int columns, int rows, Color tint) {
	const float w = 1.f / static_cast<float>(columns);
	const float h = 1.f / static_cast<float>(rows);
	const float u = w * static_cast<float>(cell % columns);
	const int row = cell / columns;
	const float v = 1.f - h * static_cast<float>(row + 1); // the PNG's first row is the top
	texturedRect(r, textureId, tint, u, v, u + w, v + h);
}

void texturedRect(const Rect& r, int textureId, Color tint, float u0, float v0, float u1, float v1) {
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textureId));
	Batch& b = batch(DefaultShader::Kind::Texture);
	b.colour(tint.r, tint.g, tint.b, 1.f);
	b.begin(Primitive::Quads);
	b.texCoord(u0, v0);
	b.vertex(r.x, r.y);
	b.texCoord(u1, v0);
	b.vertex(r.x + r.w, r.y);
	b.texCoord(u1, v1);
	b.vertex(r.x + r.w, r.y + r.h);
	b.texCoord(u0, v1);
	b.vertex(r.x, r.y + r.h);
	b.end();
}

void text(Font& font, float x, float y, const char* str, Color c, float alpha) {
	glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
	font.draw(x, y, str, {alpha, alpha, alpha, 1.f});
	glBlendFunc(GL_ONE, GL_ONE);
	font.draw(x, y, str, {c.r * alpha, c.g * alpha, c.b * alpha, 1.f});
}

void textCentered(Font& font, float cx, float y, const char* str, Color c, float alpha) {
	text(font, cx - font.TextWidth(str) / 2, y, str, c, alpha);
}

float textMiddleY(const Font& font, float cy) { return cy - font.Baseline() - font.CapHeight() / 2; }

void textMiddle(Font& font, float x, float cy, const char* str, Color c, float alpha) {
	text(font, x, textMiddleY(font, cy), str, c, alpha);
}

void textCenteredIn(Font& font, const Rect& r, const char* str, Color c, float alpha) {
	textCentered(font, r.cx(), textMiddleY(font, r.cy()), str, c, alpha);
}

std::vector<std::string> wrap(const Font& font, const std::string& text, float width) {
	std::vector<std::string> lines;
	std::string line;
	size_t pos = 0;
	while (pos < text.size()) {
		size_t end = text.find(' ', pos);
		if (end == std::string::npos)
			end = text.size();
		std::string word = text.substr(pos, end - pos);
		pos = end + 1;
		if (word.empty())
			continue;
		std::string candidate = line;
		if (!candidate.empty())
			candidate += ' ';
		candidate += word;
		if (!line.empty() && font.TextWidth(candidate.c_str()) > width) {
			lines.push_back(line);
			line = word;
		} else {
			line = candidate;
		}
	}
	if (!line.empty())
		lines.push_back(line);
	return lines;
}

void beginShapes() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void beginText() { glEnable(GL_BLEND); }

} // namespace ui
