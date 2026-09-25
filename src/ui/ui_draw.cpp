#include "ui_draw.h"
#include "../graphics/font.h"
#include <GL/gl.h>
#include <cmath>

namespace ui {

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
