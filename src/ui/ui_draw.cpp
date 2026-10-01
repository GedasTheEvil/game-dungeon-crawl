#include "ui_draw.h"
#include "../graphics/font.h"
#include <GL/gl.h>
#include <cmath>

namespace ui {

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

void fillRect(const Rect& r, Color top, Color bottom, float alpha) {
	glBegin(GL_QUADS);
	glColor4f(bottom.r, bottom.g, bottom.b, alpha);
	glVertex2f(r.x, r.y);
	glVertex2f(r.x + r.w, r.y);
	glColor4f(top.r, top.g, top.b, alpha);
	glVertex2f(r.x + r.w, r.y + r.h);
	glVertex2f(r.x, r.y + r.h);
	glEnd();
}

void strokeRect(const Rect& r, Color c, float alpha, float width) {
	glLineWidth(width);
	glColor4f(c.r, c.g, c.b, alpha);
	glBegin(GL_LINE_LOOP);
	glVertex2f(r.x, r.y);
	glVertex2f(r.x + r.w, r.y);
	glVertex2f(r.x + r.w, r.y + r.h);
	glVertex2f(r.x, r.y + r.h);
	glEnd();
	glLineWidth(1);
}

void line(float x0, float y0, float x1, float y1, Color c, float alpha, float width) {
	glLineWidth(width);
	glColor4f(c.r, c.g, c.b, alpha);
	glBegin(GL_LINES);
	glVertex2f(x0, y0);
	glVertex2f(x1, y1);
	glEnd();
	glLineWidth(1);
}

void diamond(float x, float y, float size, Color c, float alpha) {
	glColor4f(c.r, c.g, c.b, alpha);
	glBegin(GL_QUADS);
	glVertex2f(x - size, y);
	glVertex2f(x, y - size);
	glVertex2f(x + size, y);
	glVertex2f(x, y + size);
	glEnd();
}

void triangle(float x0, float y0, float x1, float y1, float x2, float y2, Color c, float alpha) {
	glColor4f(c.r, c.g, c.b, alpha);
	glBegin(GL_TRIANGLES);
	glVertex2f(x0, y0);
	glVertex2f(x1, y1);
	glVertex2f(x2, y2);
	glEnd();
}

void ring(const Rect& inner, float grow, Color c, float alphaIn, float alphaOut) {
	glBegin(GL_QUAD_STRIP);
	for (int corner = 0; corner <= 4; corner++) { // counter-clockwise from bottom left, back to the start
		float sx = corner == 1 || corner == 2 ? 1.f : 0.f;
		float sy = corner == 2 || corner == 3 ? 1.f : 0.f;
		glColor4f(c.r, c.g, c.b, alphaIn);
		glVertex2f(inner.x + sx * inner.w, inner.y + sy * inner.h);
		glColor4f(c.r, c.g, c.b, alphaOut);
		glVertex2f(inner.x + sx * inner.w + (2 * sx - 1) * grow, inner.y + sy * inner.h + (2 * sy - 1) * grow);
	}
	glEnd();
}

void ellipse(float x, float y, float rx, float ry, Color c, float alpha) {
	constexpr int SEGMENTS = 32;
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(c.r, c.g, c.b, alpha);
	glVertex2f(x, y);
	glColor4f(c.r, c.g, c.b, 0.f);
	for (int i = 0; i <= SEGMENTS; i++) {
		float a = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / SEGMENTS;
		glVertex2f(x + rx * std::cos(a), y + ry * std::sin(a));
	}
	glEnd();
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
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, 0, height, -depth, depth);
	glMatrixMode(GL_MODELVIEW);
	return width;
}

void loadScreenFonts(Font& title, Font& heading, Font& body, Font& small, float titleSize) {
	title.Load("fonts/papyrus.png", titleSize, 0.3f, true);
	heading.Load("fonts/papyrus.png", 5.f, 0.16f, true);
	body.Load("fonts/papyrus.png", 3.6f, 0.1f, true);
	small.Load("fonts/papyrus.png", 3.f, 0.08f, true);
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

// ---- screen tabs ----

namespace {
constexpr float TAB_W = 9.5f;
constexpr float TAB_H = 7.f;
constexpr float TAB_GAP = 1.f;
constexpr float TABS_RIGHT = 158.f;
constexpr float TABS_Y = 88.f; // centred on the title rule
constexpr const char* TAB_NAMES[SCREEN_TAB_COUNT] = {"Inventory", "Draft map", "Journal"};
constexpr const char* TAB_KEYS[SCREEN_TAB_COUNT] = {"I", "M", "J"};

void bar(float x0, float y0, float x1, float y1, Color c) { fillRect({x0, y0, x1 - x0, y1 - y0}, c, c, 1.f); }

// Flat glyphs in a 2s x 2s box round (cx, cy), like the menu's icons.
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
	}
}
} // namespace

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

void screenTabs(Font& small, ScreenTab open, int hovered, int held) {
	beginShapes();
	for (int tab = 0; tab < SCREEN_TAB_COUNT; tab++) {
		bool active = static_cast<int>(open) == tab;
		bool isHovered = hovered == tab && !active;
		Rect r =
			tile(screenTabRect(tab), active ? TileStyle::Lapis : TileStyle::Stone, isHovered, isHovered && held == tab);
		Color c = isHovered ? GOLD_BRIGHT : GOLD;
		tabIcon(static_cast<ScreenTab>(tab), r.x + 3.3f, r.cy(), 1.8f, c);
		beginText();
		text(small, r.x + 6.4f, r.y + 2.2f, TAB_KEYS[tab], isHovered ? TEXT_HOVER : GOLD);
		beginShapes();
	}
	if (hovered < 0)
		return;
	const char* name = TAB_NAMES[hovered];
	Rect first = screenTabRect(0);
	float w = small.TextWidth(name) + 3.f;
	Rect label = {first.x - w - 1.5f, first.y + 1.3f, w, 4.4f};
	fillRect(label, PANEL_TOP, PANEL_BOTTOM, 0.95f);
	strokeRect(label, GOLD_DIM, 1.f, 1.f);
	beginText();
	text(small, label.x + 1.5f, label.y + 1.2f, name, GOLD);
	beginShapes();
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

void texturedRect(const Rect& r, int textureId, Color tint, float u0, float v0, float u1, float v1) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(textureId));
	glColor4f(tint.r, tint.g, tint.b, 1.f);
	glBegin(GL_QUADS);
	glTexCoord2f(u0, v0);
	glVertex2f(r.x, r.y);
	glTexCoord2f(u1, v0);
	glVertex2f(r.x + r.w, r.y);
	glTexCoord2f(u1, v1);
	glVertex2f(r.x + r.w, r.y + r.h);
	glTexCoord2f(u0, v1);
	glVertex2f(r.x, r.y + r.h);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void text(Font& font, float x, float y, const char* str, Color c, float alpha) {
	glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
	glColor3f(alpha, alpha, alpha);
	font.print(x, y, "%s", str);
	glBlendFunc(GL_ONE, GL_ONE);
	glColor3f(c.r * alpha, c.g * alpha, c.b * alpha);
	font.print(x, y, "%s", str);
}

void textCentered(Font& font, float cx, float y, const char* str, Color c, float alpha) {
	text(font, cx - font.TextWidth(str) / 2, y, str, c, alpha);
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
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void beginText() {
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
}

} // namespace ui
