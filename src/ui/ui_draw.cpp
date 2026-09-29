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

void cornerStuds(const Rect& r) {
	diamond(r.x, r.y, 1.2f, GOLD, 1.f);
	diamond(r.x + r.w, r.y, 1.2f, GOLD, 1.f);
	diamond(r.x + r.w, r.y + r.h, 1.2f, GOLD, 1.f);
	diamond(r.x, r.y + r.h, 1.2f, GOLD, 1.f);
}

void panel(const Rect& r, float alpha) {
	fillRect(r, PANEL_TOP, PANEL_BOTTOM, alpha);
	strokeRect(r, BRONZE, 1.f, 3.f);
	strokeRect(r.inset(1.1f), GOLD_DIM, 0.8f, 1.f);
	cornerStuds(r);
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
