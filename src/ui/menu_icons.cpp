#include <cmath>
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include "menu_internal.h"

namespace menu_internal {
// ---- icons -----------------------------------------------------------------

void bar(float x0, float y0, float x1, float y1, Color c) { fillRect({x0, y0, x1 - x0, y1 - y0}, c, c, 1.f); }

// Open tray under the arrows of the save / load icons.
void tray(float cx, float cy, float s, Color c) {
	float t = 0.2f * s;
	bar(cx - s, cy - s, cx + s, cy - s + t, c);
	bar(cx - s, cy - s, cx - s + t, cy - 0.3f * s, c);
	bar(cx + s - t, cy - s, cx + s, cy - 0.3f * s, c);
}

// Flat glyph in a 2s x 2s box around (cx, cy).
void drawIcon(Icon icon, float cx, float cy, float s, Color c) {
	float t = 0.2f * s; // stroke
	switch (icon) {
	case Icon::Play:
		triangle(cx - 0.55f * s, cy - 0.8f * s, cx + 0.8f * s, cy, cx - 0.55f * s, cy + 0.8f * s, c, 1.f);
		break;
	case Icon::Save: // arrow down into the tray
		tray(cx, cy, s, c);
		bar(cx - t, cy - 0.05f * s, cx + t, cy + s, c);
		triangle(cx - 0.55f * s, cy, cx + 0.55f * s, cy, cx, cy - 0.6f * s, c, 1.f);
		break;
	case Icon::Load: // arrow up out of the tray
		tray(cx, cy, s, c);
		bar(cx - t, cy - 0.6f * s, cx + t, cy + 0.35f * s, c);
		triangle(cx - 0.55f * s, cy + 0.3f * s, cx + 0.55f * s, cy + 0.3f * s, cx, cy + s, c, 1.f);
		break;
	case Icon::Gear: { // eight teeth around a ring
		constexpr int TEETH = 8;
		constexpr int SEGMENTS = 24;
		float outer = 0.62f * s;
		float inner = 0.28f * s;
		glColor4f(c.r, c.g, c.b, 1.f);
		glBegin(GL_QUAD_STRIP);
		for (int i = 0; i <= SEGMENTS; i++) {
			float a = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / SEGMENTS;
			glVertex2f(cx + outer * std::cos(a), cy + outer * std::sin(a));
			glVertex2f(cx + inner * std::cos(a), cy + inner * std::sin(a));
		}
		glEnd();
		glBegin(GL_QUADS);
		for (int i = 0; i < TEETH; i++) {
			float a = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / TEETH;
			float dx = std::cos(a);
			float dy = std::sin(a);
			float w = 0.17f * s;
			float r0 = 0.5f * s;
			float r1 = 0.95f * s;
			glVertex2f(cx + dx * r0 - dy * w, cy + dy * r0 + dx * w);
			glVertex2f(cx + dx * r1 - dy * w, cy + dy * r1 + dx * w);
			glVertex2f(cx + dx * r1 + dy * w, cy + dy * r1 - dx * w);
			glVertex2f(cx + dx * r0 + dy * w, cy + dy * r0 - dx * w);
		}
		glEnd();
		break;
	}
	case Icon::Ankh: {
		constexpr int SEGMENTS = 24;
		float lx = cx;
		float ly = cy + 0.5f * s;
		glColor4f(c.r, c.g, c.b, 1.f);
		glBegin(GL_QUAD_STRIP);
		for (int i = 0; i <= SEGMENTS; i++) {
			float a = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / SEGMENTS;
			glVertex2f(lx + 0.38f * s * std::sin(a), ly + 0.48f * s * std::cos(a));
			glVertex2f(lx + (0.38f * s - t) * std::sin(a), ly + (0.48f * s - t) * std::cos(a));
		}
		glEnd();
		bar(cx - t, cy - s, cx + t, cy + 0.05f * s, c);
		bar(cx - 0.75f * s, cy - 0.12f * s, cx + 0.75f * s, cy - 0.12f * s + 1.3f * t, c);
		break;
	}
	case Icon::Exit: // door frame with an arrow walking out of it
		bar(cx - 0.9f * s, cy - s, cx - 0.9f * s + t, cy + s, c);
		bar(cx - 0.9f * s, cy + s - t, cx + 0.2f * s, cy + s, c);
		bar(cx - 0.9f * s, cy - s, cx + 0.2f * s, cy - s + t, c);
		bar(cx + 0.2f * s - t, cy + 0.4f * s, cx + 0.2f * s, cy + s, c);
		bar(cx + 0.2f * s - t, cy - s, cx + 0.2f * s, cy - 0.4f * s, c);
		bar(cx - 0.45f * s, cy - 0.8f * t, cx + 0.45f * s, cy + 0.8f * t, c);
		triangle(cx + 0.4f * s, cy - 0.5f * s, cx + s, cy, cx + 0.4f * s, cy + 0.5f * s, c, 1.f);
		break;
	case Icon::Pyramid: // lit face and shaded face
		triangle(cx - s, cy - 0.8f * s, cx + 0.15f * s, cy - 0.8f * s, cx, cy + 0.85f * s, c, 1.f);
		triangle(
			cx + 0.15f * s,
			cy - 0.8f * s,
			cx + s,
			cy - 0.8f * s,
			cx,
			cy + 0.85f * s,
			{c.r * 0.6f, c.g * 0.6f, c.b * 0.6f},
			1.f
		);
		break;
	case Icon::Back:
		bar(cx - 0.3f * s, cy - 0.8f * t, cx + 0.9f * s, cy + 0.8f * t, c);
		triangle(cx - 0.9f * s, cy, cx - 0.25f * s, cy - 0.6f * s, cx - 0.25f * s, cy + 0.6f * s, c, 1.f);
		break;
	case Icon::Book: // open book: two pages either side of the spine, with a ribbon hanging out
		bar(cx - 0.95f * s, cy - 0.6f * s, cx - 0.1f * s, cy + 0.75f * s, c);
		bar(cx + 0.1f * s, cy - 0.6f * s, cx + 0.95f * s, cy + 0.75f * s, c);
		bar(cx + 0.45f * s, cy - 0.95f * s, cx + 0.45f * s + t, cy - 0.6f * s, c);
		break;
	case Icon::KeyCap: // a key cap seen from above (How to Play, plan controls-help): the frame, a letter's stroke
		bar(cx - 0.9f * s, cy - 0.9f * s, cx + 0.9f * s, cy - 0.9f * s + t, c);
		bar(cx - 0.9f * s, cy + 0.9f * s - t, cx + 0.9f * s, cy + 0.9f * s, c);
		bar(cx - 0.9f * s, cy - 0.9f * s, cx - 0.9f * s + t, cy + 0.9f * s, c);
		bar(cx + 0.9f * s - t, cy - 0.9f * s, cx + 0.9f * s, cy + 0.9f * s, c);
		bar(cx - 0.35f * s, cy - 0.45f * s, cx - 0.35f * s + t, cy + 0.45f * s, c);
		bar(cx - 0.35f * s, cy - 0.45f * s, cx + 0.4f * s, cy - 0.45f * s + t, c);
		break;
	case Icon::Podium: // the winners' steps: second, first, third
		bar(cx - 0.95f * s, cy - 0.85f * s, cx - 0.35f * s, cy + 0.05f * s, c);
		bar(cx - 0.3f * s, cy - 0.85f * s, cx + 0.3f * s, cy + 0.6f * s, c);
		bar(cx + 0.35f * s, cy - 0.85f * s, cx + 0.95f * s, cy - 0.3f * s, c);
		break;
	}
}

// Small mouse with the used button lit, bottom left corner at (x, y), `scale` times MOUSE_W x MOUSE_H.
void drawMouse(float x, float y, MouseInput m, float scale) {
	const float w = MOUSE_W * scale;
	const float h = MOUSE_H * scale;
	constexpr float SPLIT = 0.55f; // buttons take the top 45 %
	Rect body = {x, y, w, h};
	fillRect(body, WELL, WELL, 1.f);
	float by = y + h * SPLIT;
	float third = w / 3;
	if (m == MouseInput::Left)
		fillRect({x, by, third * 1.5f, h - h * SPLIT}, GOLD, GOLD, 1.f);
	if (m == MouseInput::Right)
		fillRect({x + third * 1.5f, by, third * 1.5f, h - h * SPLIT}, GOLD, GOLD, 1.f);
	if (m == MouseInput::Middle)
		fillRect({x + third * 1.1f, by + 0.2f, third * 0.8f, h * 0.3f}, GOLD, GOLD, 1.f);
	line(x, by, x + w, by, GOLD_DIM, 1.f, 1.f);
	if (m != MouseInput::Middle)
		line(x + w / 2, by, x + w / 2, y + h, GOLD_DIM, 1.f, 1.f);
	strokeRect(body, GOLD_DIM, 1.f, 1.f);
}
} // namespace menu_internal
