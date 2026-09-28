#include "map_view.h"
#include "ui_draw.h"
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "../test/scenario.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using namespace ui;

namespace {
constexpr float CANVAS_H = 100.f;
constexpr float PAPER_H = 96.f;
constexpr float PAPER_PAD_X = 6.f;
constexpr float PAPER_PAD_TOP = 11.f; // room for the title
constexpr float PAPER_PAD_BOTTOM = 7.f;
constexpr float MAX_CELL = 5.f;			 // zoom limit for a small explored part, canvas units per cell
constexpr float JITTER = 0.07f;			 // pencil wobble, in cells
constexpr float REFERENCE_RES_Y = 720.f; // line widths are in pixels at this height and this cell size
constexpr float REFERENCE_CELL = 2.f;

constexpr Color GRAPHITE = {0.22f, 0.21f, 0.20f};
constexpr Color PENCIL_RED = {0.72f, 0.14f, 0.08f};
constexpr Color LOCK_PENCILS[LOCK_COLOUR_COUNT] = {
	{0.75f, 0.16f, 0.10f}, // red
	{0.12f, 0.27f, 0.62f}, // blue
	{0.14f, 0.48f, 0.18f}, // green
	{0.80f, 0.58f, 0.08f}, // gold
};
constexpr Color ANKH_GOLD = {0.78f, 0.56f, 0.10f};

// Stable per-stroke randomness, so the sketch does not crawl from frame to frame.
float noise(uint32_t seed) {
	seed ^= seed >> 16;
	seed *= 0x7feb352dU;
	seed ^= seed >> 15;
	seed *= 0x846ca68bU;
	seed ^= seed >> 16;
	return static_cast<float>(seed & 0xffffU) / 32767.5f - 1.f; // [-1, 1]
}

struct Stroke {
	float x0, y0, x1, y1;
	Color c;
	float alpha;
};

// Collects pencil strokes in map cell units, drawn in batches of one line width.
class Sketch {
  public:
	Sketch(float originX, float originY, float cell) : ox(originX), oy(originY), s(cell) {}

	// A hand-drawn line: slightly off at both ends, gone over twice with less pressure the second time.
	void pencil(float x0, float y0, float x1, float y1, Color c, float alpha, uint32_t seed) {
		for (uint32_t pass = 0; pass < 2; pass++) {
			uint32_t k = seed * 8 + pass * 4;
			strokes.push_back({ox + (x0 + JITTER * noise(k)) * s, oy + (y0 + JITTER * noise(k + 1)) * s,
							   ox + (x1 + JITTER * noise(k + 2)) * s, oy + (y1 + JITTER * noise(k + 3)) * s, c,
							   pass == 0 ? alpha : alpha * 0.45f});
		}
	}
	// Closed polygon through `n` points on an ellipse round (cx, cy).
	void loop(float cx, float cy, float rx, float ry, int n, Color c, float alpha, uint32_t seed) {
		for (int i = 0; i < n; i++) {
			float a0 = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(n);
			float a1 = 2.f * static_cast<float>(M_PI) * static_cast<float>(i + 1) / static_cast<float>(n);
			pencil(cx + rx * std::cos(a0), cy + ry * std::sin(a0), cx + rx * std::cos(a1), cy + ry * std::sin(a1), c,
				   alpha, seed + static_cast<uint32_t>(i));
		}
	}
	void flush(float widthPx) {
		glLineWidth(widthPx);
		glBegin(GL_LINES);
		for (const Stroke& st : strokes) {
			glColor4f(st.c.r, st.c.g, st.c.b, st.alpha);
			glVertex2f(st.x0, st.y0);
			glVertex2f(st.x1, st.y1);
		}
		glEnd();
		glLineWidth(1);
		strokes.clear();
	}

  private:
	float ox, oy, s;
	std::vector<Stroke> strokes;
};

uint32_t cellSeed(int i, int j, int part) {
	return static_cast<uint32_t>((j * LEVEL_WIDTH + i) * 64 + part) * 2654435761U;
}

Color lockPencil(int colour) { return isLockColour(colour) ? LOCK_PENCILS[colour - 1] : GRAPHITE; }

// Diagonal hatching over a wall cell; lines of neighbouring cells meet, so walls read as one shaded mass.
void hatch(Sketch& sk, int i, int j) {
	auto x = static_cast<float>(i);
	auto y = static_cast<float>(j);
	constexpr float STEPS[] = {0.5f, 1.f, 1.5f, 2.f};
	for (int k = 0; k < 4; k++) {
		float t = STEPS[k];
		if (t <= 1.f)
			sk.pencil(x + t, y, x, y + t, GRAPHITE, 0.30f, cellSeed(i, j, k));
		else
			sk.pencil(x + 1, y + t - 1, x + t - 1, y + 1, GRAPHITE, 0.30f, cellSeed(i, j, k));
	}
}

// Outline between an explored open cell and the explored walls around it.
void outline(Sketch& sk, const Dungeon& d, int i, int j) {
	auto x = static_cast<float>(i);
	auto y = static_cast<float>(j);
	auto wall = [&](int ci, int cj) { return d.Explored(ci, cj) && d.Cell(ci, cj).type == Wall; };
	// Seeds come from the edge, not the cell, so a shared edge looks the same whichever side draws it.
	if (wall(i - 1, j))
		sk.pencil(x, y, x, y + 1, GRAPHITE, 0.85f, cellSeed(i, j, 10));
	if (wall(i + 1, j))
		sk.pencil(x + 1, y, x + 1, y + 1, GRAPHITE, 0.85f, cellSeed(i + 1, j, 10));
	if (wall(i, j - 1))
		sk.pencil(x, y, x + 1, y, GRAPHITE, 0.85f, cellSeed(i, j, 11));
	if (wall(i, j + 1))
		sk.pencil(x, y + 1, x + 1, y + 1, GRAPHITE, 0.85f, cellSeed(i, j + 1, 11));
}

void arch(Sketch& sk, float x, float y, Color c, uint32_t seed) {
	sk.pencil(x + 0.22f, y, x + 0.22f, y + 0.55f, c, 0.9f, seed);
	sk.pencil(x + 0.78f, y, x + 0.78f, y + 0.55f, c, 0.9f, seed + 1);
	constexpr int SEGMENTS = 4;
	for (int k = 0; k < SEGMENTS; k++) {
		float a0 = static_cast<float>(M_PI) * static_cast<float>(k) / SEGMENTS;
		float a1 = static_cast<float>(M_PI) * static_cast<float>(k + 1) / SEGMENTS;
		sk.pencil(x + 0.5f + 0.28f * std::cos(a0), y + 0.55f + 0.3f * std::sin(a0), x + 0.5f + 0.28f * std::cos(a1),
				  y + 0.55f + 0.3f * std::sin(a1), c, 0.9f, seed + 2 + static_cast<uint32_t>(k));
	}
}

void zigzag(Sketch& sk, float x, float y, float height, Color c, uint32_t seed) {
	constexpr int TEETH = 3;
	for (int k = 0; k < TEETH; k++) {
		float left = x + 0.1f + 0.8f * static_cast<float>(k) / TEETH;
		float right = x + 0.1f + 0.8f * static_cast<float>(k + 1) / TEETH;
		float tip = (left + right) / 2;
		sk.pencil(left, y + 0.05f, tip, y + height, c, 0.9f, seed + static_cast<uint32_t>(2 * k));
		sk.pencil(tip, y + height, right, y + 0.05f, c, 0.9f, seed + static_cast<uint32_t>(2 * k + 1));
	}
}

void symbol(Sketch& sk, int i, int j, Tile t) {
	auto x = static_cast<float>(i);
	auto y = static_cast<float>(j);
	uint32_t seed = cellSeed(i, j, 20);
	switch (t.type) {
	case Ladder:
		sk.pencil(x + 0.3f, y, x + 0.3f, y + 1, GRAPHITE, 0.8f, seed);
		sk.pencil(x + 0.7f, y, x + 0.7f, y + 1, GRAPHITE, 0.8f, seed + 1);
		for (int k = 0; k < 3; k++) {
			float v = y + 0.2f + 0.3f * static_cast<float>(k);
			sk.pencil(x + 0.3f, v, x + 0.7f, v, GRAPHITE, 0.8f, seed + 2 + static_cast<uint32_t>(k));
		}
		break;
	case Door:
		arch(sk, x, y, t.attr == GateExit ? PENCIL_RED : (t.attr == GateRiddle ? LOCK_PENCILS[1] : GRAPHITE), seed);
		break;
	case Treasure: // X marks the spot
		sk.pencil(x + 0.25f, y + 0.2f, x + 0.75f, y + 0.7f, PENCIL_RED, 0.95f, seed);
		sk.pencil(x + 0.25f, y + 0.7f, x + 0.75f, y + 0.2f, PENCIL_RED, 0.95f, seed + 1);
		break;
	case Spike:
		zigzag(sk, x, y, 0.4f, GRAPHITE, seed);
		break;
	case Death:
		zigzag(sk, x, y, 0.65f, PENCIL_RED, seed);
		break;
	case Key: {
		Color c = lockPencil(t.attr);
		sk.loop(x + 0.3f, y + 0.5f, 0.14f, 0.14f, 6, c, 0.95f, seed);
		sk.pencil(x + 0.44f, y + 0.5f, x + 0.85f, y + 0.5f, c, 0.95f, seed + 6);
		sk.pencil(x + 0.72f, y + 0.5f, x + 0.72f, y + 0.35f, c, 0.95f, seed + 7);
		sk.pencil(x + 0.84f, y + 0.5f, x + 0.84f, y + 0.35f, c, 0.95f, seed + 8);
		break;
	}
	case Gate: {
		Color c = lockPencil(t.attr);
		float bottom = t.value == 1 ? 0.75f : 0.f; // an open gate hangs up in the ceiling
		sk.pencil(x + 0.15f, y + 0.95f, x + 0.85f, y + 0.95f, c, 0.95f, seed);
		if (t.value != 1)
			sk.pencil(x + 0.15f, y + 0.05f, x + 0.85f, y + 0.05f, c, 0.95f, seed + 1);
		for (int k = 0; k < 3; k++) {
			float u = x + 0.3f + 0.2f * static_cast<float>(k);
			sk.pencil(u, y + bottom, u, y + 0.95f, c, 0.95f, seed + 2 + static_cast<uint32_t>(k));
		}
		break;
	}
	case Lever: {
		Color c = lockPencil(t.attr);
		float tipX = t.value == 1 ? 0.25f : 0.75f;
		sk.pencil(x + 0.25f, y + 0.08f, x + 0.75f, y + 0.08f, c, 0.95f, seed);
		sk.pencil(x + 0.5f, y + 0.08f, x + tipX, y + 0.55f, c, 0.95f, seed + 1);
		sk.loop(x + tipX, y + 0.6f, 0.07f, 0.07f, 5, c, 0.95f, seed + 2);
		break;
	}
	case RockFall:
		if (t.value == 1) { // fallen: a boulder on the floor
			sk.loop(x + 0.5f, y + 0.25f, 0.28f, 0.2f, 7, GRAPHITE, 0.9f, seed);
		} else { // loose ceiling: a crack
			sk.pencil(x + 0.15f, y + 0.95f, x + 0.35f, y + 0.78f, GRAPHITE, 0.9f, seed);
			sk.pencil(x + 0.35f, y + 0.78f, x + 0.55f, y + 0.9f, GRAPHITE, 0.9f, seed + 1);
			sk.pencil(x + 0.55f, y + 0.9f, x + 0.8f, y + 0.72f, GRAPHITE, 0.9f, seed + 2);
		}
		break;
	case Ankh:
		sk.loop(x + 0.5f, y + 0.74f, 0.12f, 0.16f, 7, ANKH_GOLD, 0.95f, seed);
		sk.pencil(x + 0.28f, y + 0.55f, x + 0.72f, y + 0.55f, ANKH_GOLD, 0.95f, seed + 7);
		sk.pencil(x + 0.5f, y + 0.58f, x + 0.5f, y + 0.08f, ANKH_GOLD, 0.95f, seed + 8);
		break;
	default:
		break;
	}
}

void drawBackground(float canvasW) {
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	Game().assets.textures.loadingBackground.Bind();
	glColor3f(0.30f, 0.24f, 0.18f);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	glTexCoord2f(1, 0);
	glVertex2f(canvasW, 0);
	glTexCoord2f(1, 1);
	glVertex2f(canvasW, CANVAS_H);
	glTexCoord2f(0, 1);
	glVertex2f(0, CANVAS_H);
	glEnd();
	beginShapes();
	constexpr float VIGNETTE = 20.f;
	ring(Rect{0, 0, canvasW, CANVAS_H}.inset(VIGNETTE), VIGNETTE, BLACK, 0.f, 0.85f);
}

// The papyrus sheet is landscape, the map portrait: the texture is turned a quarter round.
void drawPaper(const Rect& r) {
	beginShapes();
	fillRect({r.x + 1.2f, r.y - 1.2f, r.w, r.h}, BLACK, BLACK, 0.45f); // shadow
	glEnable(GL_TEXTURE_2D);
	Game().assets.textures.papyrus.Bind();
	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(1, 0);
	glVertex2f(r.x, r.y);
	glTexCoord2f(1, 1);
	glVertex2f(r.x + r.w, r.y);
	glTexCoord2f(0, 1);
	glVertex2f(r.x + r.w, r.y + r.h);
	glTexCoord2f(0, 0);
	glVertex2f(r.x, r.y + r.h);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}
} // namespace

void DraftMap::Draw() {
	const Dungeon& d = Game().dungeon;
	float canvasW = CANVAS_H * static_cast<float>(Game().render.resX) / static_cast<float>(Game().render.resY);
	float resScale = static_cast<float>(Game().render.resY) / REFERENCE_RES_Y;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, canvasW, 0, CANVAS_H, -21, 21);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	drawBackground(canvasW);

	// The sheet fits the whole level; the sketch is scaled up to fill it with the part explored so far.
	float drawH = PAPER_H - PAPER_PAD_TOP - PAPER_PAD_BOTTOM;
	float drawW = drawH / LEVEL_HEIGHT * LEVEL_WIDTH;
	Rect paper = {(canvasW - drawW) / 2 - PAPER_PAD_X, (CANVAS_H - PAPER_H) / 2, drawW + 2 * PAPER_PAD_X, PAPER_H};
	drawPaper(paper);
	Rect area = {paper.x + PAPER_PAD_X, paper.y + PAPER_PAD_BOTTOM, drawW, drawH};

	int minI = LEVEL_WIDTH, maxI = -1, minJ = LEVEL_HEIGHT, maxJ = -1;
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j)) {
				minI = std::min(minI, i);
				maxI = std::max(maxI, i);
				minJ = std::min(minJ, j);
				maxJ = std::max(maxJ, j);
			}
	if (maxI < 0) { // nothing explored: the whole level
		minI = minJ = 0;
		maxI = LEVEL_WIDTH - 1;
		maxJ = LEVEL_HEIGHT - 1;
	}
	auto spanI = static_cast<float>(maxI - minI + 1);
	auto spanJ = static_cast<float>(maxJ - minJ + 1);
	float cell = std::min({area.w / spanI, area.h / spanJ, MAX_CELL});
	float originX = area.cx() - (static_cast<float>(minI) + spanI / 2) * cell;
	float originY = area.cy() - (static_cast<float>(minJ) + spanJ / 2) * cell;

	float pxScale = resScale * std::sqrt(cell / REFERENCE_CELL);
	beginShapes();
	glEnable(GL_LINE_SMOOTH);
	Sketch sk(originX, originY, cell);
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j) && d.Cell(i, j).type == Wall)
				hatch(sk, i, j);
	sk.flush(1.f * pxScale);

	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j) && d.Cell(i, j).type != Wall)
				outline(sk, d, i, j);
	sk.flush(2.f * pxScale);

	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j))
				symbol(sk, i, j, d.Cell(i, j));
	sk.flush(1.5f * pxScale);

	// "You are here": a red ring round the archaeologist.
	float px, py;
	Game().dungeon.getC(px, py);
	sk.loop(px, py + 0.5f, 0.6f, 0.6f, 9, PENCIL_RED, 1.f, 7u);
	sk.flush(2.f * pxScale);
	diamond(originX + px * cell, originY + (py + 0.5f) * cell, cell * 0.22f, PENCIL_RED, 1.f);
	glDisable(GL_LINE_SMOOTH);

	std::string title = "Level " + std::to_string(Game().curMap);
	beginText();
	textCentered(Game().assets.fonts.status, paper.cx(), paper.y + paper.h - PAPER_PAD_TOP + 2.5f, title.c_str(), INK);
	textCentered(Game().assets.fonts.font, paper.cx(), paper.y + 2.f, "M / Esc  close", INK_FADED);

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
	glFlush();

	Scenario::onFrameRendered();
	glutSwapBuffers();
}
