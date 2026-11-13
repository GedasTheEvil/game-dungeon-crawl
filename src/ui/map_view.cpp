#include "map_view.h"
#include "../level/map_marks.h"
#include "../render/ui_draw.h"
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "../level/bindings.h"
#include "../level/input.h"
#include "../render/font.h"
#include <GL/gl.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <utility>

using namespace ui;

namespace {
constexpr float PAPER_H = 96.f;
constexpr float PAPER_PAD_X = 6.f;
constexpr float PAPER_PAD_TOP = 11.f; // room for the title
constexpr float PAPER_PAD_BOTTOM = 7.f;
constexpr float MAX_CELL = 5.f; // zoom limit for a small explored part, canvas units per cell
constexpr float JITTER = 0.07f; // pencil wobble, in cells
constexpr float REFERENCE_RES_Y = 720.f; // line widths are in pixels at this height and this cell size
constexpr float REFERENCE_CELL = 2.f;

constexpr Color GRAPHITE = PENCIL;
constexpr Color PENCIL_RED = {0.72f, 0.14f, 0.08f};
constexpr Color LOCK_PENCILS[LOCK_COLOUR_COUNT] = {
	{0.75f, 0.16f, 0.10f}, // red
	{0.12f, 0.27f, 0.62f}, // blue
	{0.14f, 0.48f, 0.18f}, // green
	{0.80f, 0.58f, 0.08f}, // gold
};
constexpr Color WATER_BLUE = {0.16f, 0.38f, 0.70f};
constexpr float WAVE_AMPLITUDE = 0.06f; // cells
constexpr int WAVE_SEGMENTS = 8; // per wave, two crests

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

	// A hand-drawn line: slightly off at both ends (by up to jitter cells), gone over twice with less pressure the
	// second time.
	void pencil(float x0, float y0, float x1, float y1, Color c, float alpha, uint32_t seed, float jitter = JITTER) {
		for (uint32_t pass = 0; pass < 2; pass++) {
			uint32_t k = seed * 8 + pass * 4;
			strokes.push_back(
				{ox + (x0 + jitter * noise(k)) * s,
				 oy + (y0 + jitter * noise(k + 1)) * s,
				 ox + (x1 + jitter * noise(k + 2)) * s,
				 oy + (y1 + jitter * noise(k + 3)) * s,
				 c,
				 pass == 0 ? alpha : alpha * 0.45f}
			);
		}
	}
	// Closed polygon through `n` points on an ellipse round (cx, cy).
	void loop(float cx, float cy, float rx, float ry, int n, Color c, float alpha, uint32_t seed) {
		for (int i = 0; i < n; i++) {
			float a0 = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(n);
			float a1 = 2.f * static_cast<float>(M_PI) * static_cast<float>(i + 1) / static_cast<float>(n);
			pencil(
				cx + rx * std::cos(a0),
				cy + ry * std::sin(a0),
				cx + rx * std::cos(a1),
				cy + ry * std::sin(a1),
				c,
				alpha,
				seed + static_cast<uint32_t>(i)
			);
		}
	}
	// A wavy line ("~~") from x0 to x1 round height y: two crests, the whole wave a little off as drawn by hand.
	void wave(float x0, float x1, float y, Color c, float alpha, uint32_t seed) {
		const float dx = JITTER * noise(seed) * 0.5f;
		const float dy = JITTER * noise(seed + 1) * 0.5f;
		for (uint32_t pass = 0; pass < 2; pass++) {
			const float lift = pass == 0 ? 0.f : 0.012f;
			for (int k = 0; k < WAVE_SEGMENTS; k++) {
				const float t0 = static_cast<float>(k) / WAVE_SEGMENTS;
				const float t1 = static_cast<float>(k + 1) / WAVE_SEGMENTS;
				const auto yAt = [&](float t) {
					return y + dy + lift + WAVE_AMPLITUDE * std::sin(t * 4.f * static_cast<float>(M_PI));
				};
				strokes.push_back(
					{ox + (x0 + dx + (x1 - x0) * t0) * s,
					 oy + yAt(t0) * s,
					 ox + (x0 + dx + (x1 - x0) * t1) * s,
					 oy + yAt(t1) * s,
					 c,
					 pass == 0 ? alpha : alpha * 0.45f}
				);
			}
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

// The jitter of the sub-cells' lines: a quarter of a cell is small for the walls' wobble.
constexpr float SUBCELL_JITTER = JITTER / 2;

// The solid sub-cells of a shaped cell (plan level-subcells): the walls' hatching, twice as dense (the lines x + y on
// the quarter tile) so that a quarter step reads as rock too, cut to each solid sub-cell.
void hatchShape(Sketch& sk, int i, int j, const Tile& t) {
	constexpr int LINES = 2 * SUBCELLS - 1;
	for (int row = 0; row < SUBCELLS; row++)
		for (int col = 0; col < SUBCELLS; col++) {
			if (!subcellSolid(t, col, row))
				continue;
			const float x0 = static_cast<float>(col) * SUBCELL_SIZE;
			const float y0 = static_cast<float>(row) * SUBCELL_SIZE;
			for (int k = 1; k <= LINES; k++) { // the line x + y = c through the sub-cell, from its lower right end
				const float c = static_cast<float>(k) * SUBCELL_SIZE;
				const float xa = std::max(x0, c - y0 - SUBCELL_SIZE);
				const float xb = std::min(x0 + SUBCELL_SIZE, c - y0);
				if (xa < xb)
					sk.pencil(
						static_cast<float>(i) + xb,
						static_cast<float>(j) + c - xb,
						static_cast<float>(i) + xa,
						static_cast<float>(j) + c - xa,
						GRAPHITE,
						0.30f,
						cellSeed(i, j, 32 + k) ^ (subcellBit(col, row) * 0x9e3779b9U),
						SUBCELL_JITTER
					);
			}
		}
}

// The outline of a shaped cell: where a solid sub-cell meets an open one. A neighbour without a shape (rock or open
// space) draws none of its own, so the shaped cell draws every edge against it; between two shaped cells the solid
// side draws.
void outlineShape(Sketch& sk, const Dungeon& d, int i, int j) {
	const Tile t = d.Cell(i, j);
	constexpr int DIRS[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
	for (int row = 0; row < SUBCELLS; row++)
		for (int col = 0; col < SUBCELLS; col++) {
			const bool solid = subcellSolid(t, col, row);
			for (const auto& dir : DIRS) {
				int nc = col + dir[0], nr = row + dir[1], ci = i, cj = j;
				if (nc < 0 || nc >= SUBCELLS || nr < 0 || nr >= SUBCELLS) { // the sub-cell in the next cell
					ci += dir[0];
					cj += dir[1];
					nc = (nc + SUBCELLS) % SUBCELLS;
					nr = (nr + SUBCELLS) % SUBCELLS;
					if (!d.Explored(ci, cj))
						continue;
				}
				const Tile n = d.Cell(ci, cj);
				const bool plainNeighbour = (ci != i || cj != j) && !hasShape(n);
				if (solid == subcellSolid(n, nc, nr) || (!solid && !plainNeighbour))
					continue;
				// The edge between the two sub-cells, in map cells.
				const float x = static_cast<float>(i) + static_cast<float>(col) * SUBCELL_SIZE;
				const float y = static_cast<float>(j) + static_cast<float>(row) * SUBCELL_SIZE;
				const float ex = x + (dir[0] > 0 ? SUBCELL_SIZE : 0.f);
				const float ey = y + (dir[1] > 0 ? SUBCELL_SIZE : 0.f);
				const uint32_t seed = cellSeed(i, j, 40) ^ (subcellBit(col, row) * 0x85ebca6bU) ^
									  static_cast<uint32_t>(dir[0] + 2 * dir[1] + 3);
				if (dir[0] != 0)
					sk.pencil(ex, y, ex, y + SUBCELL_SIZE, GRAPHITE, 0.85f, seed, SUBCELL_JITTER);
				else
					sk.pencil(x, ey, x + SUBCELL_SIZE, ey, GRAPHITE, 0.85f, seed, SUBCELL_JITTER);
			}
		}
}

// Water: blue waves in the lower part of a half-water cell (its water line and below), three rows over a deep one.
void water(Sketch& sk, int i, int j, Tile t) {
	auto x = static_cast<float>(i);
	auto y = static_cast<float>(j);
	if (inHalfWater(t)) {
		sk.wave(x + 0.1f, x + 0.9f, y + 0.4f, WATER_BLUE, 0.75f, cellSeed(i, j, 20));
		sk.wave(x + 0.25f, x + 0.75f, y + 0.2f, WATER_BLUE, 0.5f, cellSeed(i, j, 21));
	} else if (t.structure == Structure::DeepWater) {
		for (int k = 0; k < 3; k++)
			sk.wave(
				x + 0.1f,
				x + 0.9f,
				y + 0.22f + 0.28f * static_cast<float>(k),
				WATER_BLUE,
				0.8f,
				cellSeed(i, j, 22 + k)
			);
	}
}

// Deep water blocks like a wall: a blue line where an explored open cell meets it.
void waterEdge(Sketch& sk, const Dungeon& d, int i, int j) {
	auto x = static_cast<float>(i);
	auto y = static_cast<float>(j);
	auto deep = [&](int ci, int cj) { return d.Explored(ci, cj) && d.Cell(ci, cj).structure == Structure::DeepWater; };
	if (deep(i - 1, j))
		sk.pencil(x, y, x, y + 1, WATER_BLUE, 0.8f, cellSeed(i, j, 26));
	if (deep(i + 1, j))
		sk.pencil(x + 1, y, x + 1, y + 1, WATER_BLUE, 0.8f, cellSeed(i + 1, j, 26));
	if (deep(i, j - 1))
		sk.pencil(x, y, x + 1, y, WATER_BLUE, 0.8f, cellSeed(i, j, 27));
	if (deep(i, j + 1))
		sk.pencil(x, y + 1, x + 1, y + 1, WATER_BLUE, 0.8f, cellSeed(i, j + 1, 27));
}

// Outline between an explored open cell and the explored walls around it.
void outline(Sketch& sk, const Dungeon& d, int i, int j) {
	auto x = static_cast<float>(i);
	auto y = static_cast<float>(j);
	auto wall = [&](int ci, int cj) { return d.Explored(ci, cj) && isWall(d.Cell(ci, cj)); };
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

// One symbol from the sheet (media/textures/ui/map_marks.png, plan map-legend-sprites) over the cell whose lower left
// corner is (x, y), `cell` wide, in canvas units. Between beginMarks and endMarks.
void markQuad(float x, float y, float cell, const ShownMark& shown) {
	const MarkSprite sprite = markSprite(shown);
	const float w = 1.f / MAP_MARK_COLUMNS;
	const float h = 1.f / MAP_MARK_ROWS;
	const float u = w * static_cast<float>(sprite.cell % MAP_MARK_COLUMNS);
	const int row = sprite.cell / MAP_MARK_COLUMNS;
	const float v = 1.f - h * static_cast<float>(row + 1); // the PNG's first row is the top
	// The sprite's corners from its lower left one, counterclockwise; a quarter turn clockwise shifts them by one.
	const float corners[4][2] = {{u, v}, {u + w, v}, {u + w, v + h}, {u, v + h}};
	const float size = cell / (1.f - 2.f * MAP_MARK_MARGIN); // the sprite's margin lies round the cell
	const float left = x - size * MAP_MARK_MARGIN;
	const float bottom = y - size * MAP_MARK_MARGIN;
	const float quad[4][2] =
		{{left, bottom}, {left + size, bottom}, {left + size, bottom + size}, {left, bottom + size}};
	const Color tint = markDef(shown.mark).tint == MarkTint::Lock ? lockPencil(sprite.lock) : Color{1.f, 1.f, 1.f};
	glColor4f(tint.r, tint.g, tint.b, 1.f);
	for (int k = 0; k < 4; k++) {
		const float* uv = corners[(k + sprite.turns) % 4];
		glTexCoord2f(uv[0], uv[1]);
		glVertex2f(quad[k][0], quad[k][1]);
	}
}

void beginMarks() {
	glEnable(GL_TEXTURE_2D);
	Game().assets.textures.mapMarks.Bind();
	glBegin(GL_QUADS);
}

void endMarks() {
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

// What a cell shows on the map: its object, else its fixture, else a taken treasure's container, else its prop.
ShownMark shownMark(const Dungeon& d, int i, int j) {
	return cellMark(d.Cell(i, j), d.PropAt(i, j), d.ContainerEmptied(i, j));
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
// ---- legend and zoom (plan map-symbols-and-zoom) ----
constexpr float LEGEND_ROW_H = 4.f; // at the most; a long legend packs its rows closer
constexpr float LEGEND_CELL = 2.8f; // a symbol's cell
constexpr float LEGEND_PAD = 2.5f;
constexpr float LEGEND_TEXT_GAP = 1.4f; // symbol to name
constexpr float LEGEND_HEAD = 8.f; // the heading, from the slip's top to the first row
constexpr float LEGEND_GAP = 2.f; // slip to sheet
constexpr float LEGEND_MARGIN = 2.f; // slip to the window's left edge
constexpr float CLIP_BOTTOM = 5.5f; // the sketch's clip, from the sheet's bottom: over the key hint
constexpr float CLIP_TOP = 9.f; // ... from its top: under the title
constexpr float CLIP_SIDE = 1.5f;
constexpr float PAN_STEP = 10.f; // canvas units an arrow press
constexpr const char* YOU = "You";

using LegendRow = ShownMark; // a mark, and the first explored thing with it: the row draws its symbol

// The marks on the explored cells, in MapMark order.
std::vector<LegendRow> legendRows(const Dungeon& d) {
	std::array<bool, MARK_COUNT> seen{};
	std::array<ShownMark, MARK_COUNT> samples{};
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j)) {
				const ShownMark shown = shownMark(d, i, j);
				if (shown.mark != MapMark::None && !seen[std::to_underlying(shown.mark)]) {
					seen[std::to_underlying(shown.mark)] = true;
					samples[std::to_underlying(shown.mark)] = shown;
				}
			}
	std::vector<LegendRow> rows;
	for (size_t k = 0; k < seen.size(); k++)
		if (seen[k])
			rows.push_back(samples[k]);
	return rows;
}

// One row per mark, then the red ring of "you are here".
void drawLegend(const Rect& slip, const std::vector<LegendRow>& rows, float rowH, float resScale) {
	drawPaper(slip);
	const float cell = std::min(LEGEND_CELL, rowH * 0.85f);
	const float top = slip.y + slip.h - LEGEND_HEAD;
	const auto rowBottom = [&](size_t k) { return top - rowH * static_cast<float>(k + 1); };
	beginShapes();
	beginMarks();
	for (size_t k = 0; k < rows.size(); k++)
		markQuad(slip.x + LEGEND_PAD, rowBottom(k) + (rowH - cell) / 2, cell, rows[k]);
	endMarks();
	glEnable(GL_LINE_SMOOTH);
	Sketch you(slip.x + LEGEND_PAD, rowBottom(rows.size()) + (rowH - cell) / 2, cell);
	you.loop(0.5f, 0.5f, 0.45f, 0.45f, 9, PENCIL_RED, 1.f, 7u);
	you.flush(1.5f * resScale * std::sqrt(cell / REFERENCE_CELL));
	diamond(slip.x + LEGEND_PAD + cell / 2, rowBottom(rows.size()) + rowH / 2, cell * 0.17f, PENCIL_RED, 1.f);
	glDisable(GL_LINE_SMOOTH);

	beginText();
	Font& font = Game().assets.fonts.hudSmall;
	textCentered(font, slip.cx(), slip.y + slip.h - LEGEND_HEAD + 2.f, "Legend", INK);
	for (size_t k = 0; k <= rows.size(); k++)
		textMiddle(
			font,
			slip.x + LEGEND_PAD + cell + LEGEND_TEXT_GAP,
			rowBottom(k) + rowH / 2,
			k < rows.size() ? markDef(rows[k].mark).name : YOU,
			INK
		);
}

// Clips the sketch to `r` (canvas units, the modelview is the identity), so a zoomed one stays on the sheet.
void clipTo(const Rect& r) {
	const GLdouble planes[4][4] = {
		{1, 0, 0, -r.x},
		{-1, 0, 0, r.x + r.w},
		{0, 1, 0, -r.y},
		{0, -1, 0, r.y + r.h},
	};
	for (int k = 0; k < 4; k++) {
		glClipPlane(static_cast<GLenum>(GL_CLIP_PLANE0 + k), planes[k]);
		glEnable(static_cast<GLenum>(GL_CLIP_PLANE0 + k));
	}
}

// The arrows and the bound move keys: the view goes that way, the sketch the other.
void pan(MapZoom& zoom, const MapFit& fit, NavKey key) {
	switch (key) {
	case NavKey::Left:
		zoom.Pan(fit, PAN_STEP, 0.f);
		break;
	case NavKey::Right:
		zoom.Pan(fit, -PAN_STEP, 0.f);
		break;
	case NavKey::Up:
		zoom.Pan(fit, 0.f, -PAN_STEP);
		break;
	case NavKey::Down:
		zoom.Pan(fit, 0.f, PAN_STEP);
		break;
	case NavKey::Activate:
	case NavKey::None:
		break;
	}
}

void unclip() {
	for (int k = 0; k < 4; k++)
		glDisable(static_cast<GLenum>(GL_CLIP_PLANE0 + k));
}
} // namespace

void DraftMap::Draw() {
	const Dungeon& d = Game().dungeon;
	float resScale = static_cast<float>(Game().render.resY) / REFERENCE_RES_Y;
	if (d.LevelNumber() != level) {
		level = d.LevelNumber();
		zoom.Reset();
		dragging = false;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	canvasW = beginSquareCanvas(CANVAS_H, Game().render.resX, Game().render.resY);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	drawBackground(canvasW);

	// The sheet fits the whole level; the sketch is scaled up to fill it with the part explored so far.
	float drawH = PAPER_H - PAPER_PAD_TOP - PAPER_PAD_BOTTOM;
	float drawW = drawH / LEVEL_HEIGHT * LEVEL_WIDTH;
	Rect paper = {(canvasW - drawW) / 2 - PAPER_PAD_X, (CANVAS_H - PAPER_H) / 2, drawW + 2 * PAPER_PAD_X, PAPER_H};
	drawPaper(paper);

	// The legend slip left of the sheet; in a narrow window it lies over the sheet's margin.
	const std::vector<LegendRow> rows = legendRows(d);
	Font& small = Game().assets.fonts.hudSmall;
	float namesW = std::max(small.TextWidth("Legend"), small.TextWidth(YOU));
	for (const LegendRow& row : rows)
		namesW = std::max(namesW, small.TextWidth(markDef(row.mark).name));
	const auto rowCount = static_cast<float>(rows.size() + 1);
	const float rowH = std::min(LEGEND_ROW_H, (PAPER_H - LEGEND_HEAD - LEGEND_PAD) / rowCount);
	const float slipW = 2 * LEGEND_PAD + std::min(LEGEND_CELL, rowH * 0.85f) + LEGEND_TEXT_GAP + namesW;
	const float slipH = LEGEND_HEAD + rowH * rowCount + LEGEND_PAD;
	const float slipX = std::max(LEGEND_MARGIN, paper.x - LEGEND_GAP - slipW);
	const Rect slip = {slipX, paper.y + paper.h - slipH, slipW, slipH};
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
	fit = {
		area,
		std::min({area.w / spanI, area.h / spanJ, MAX_CELL}),
		static_cast<float>(minI) + spanI / 2,
		static_cast<float>(minJ) + spanJ / 2
	};
	const MapFrame frame = zoom.Frame(fit);
	const float cell = frame.cell;
	const float originX = frame.originX;
	const float originY = frame.originY;

	float pxScale = resScale * std::sqrt(cell / REFERENCE_CELL);
	beginShapes();
	clipTo({paper.x + CLIP_SIDE, paper.y + CLIP_BOTTOM, paper.w - 2 * CLIP_SIDE, paper.h - CLIP_BOTTOM - CLIP_TOP});
	glEnable(GL_LINE_SMOOTH);
	Sketch sk(originX, originY, cell);
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j) && isWall(d.Cell(i, j)))
				hatch(sk, i, j);
			else if (d.Explored(i, j) && hasShape(d.Cell(i, j)))
				hatchShape(sk, i, j, d.Cell(i, j));
	sk.flush(1.f * pxScale);

	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j) && hasShape(d.Cell(i, j)))
				outlineShape(sk, d, i, j);
			else if (d.Explored(i, j) && !isWall(d.Cell(i, j)))
				outline(sk, d, i, j);
	sk.flush(2.f * pxScale);

	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j)) {
				water(sk, i, j, d.Cell(i, j));
				if (!isWall(d.Cell(i, j)) && d.Cell(i, j).structure != Structure::DeepWater)
					waterEdge(sk, d, i, j);
			}
	sk.flush(1.5f * pxScale);

	glDisable(GL_LINE_SMOOTH);
	beginMarks(); // one symbol a cell
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j))
				if (const ShownMark shown = shownMark(d, i, j); shown.mark != MapMark::None)
					markQuad(
						originX + static_cast<float>(i) * cell,
						originY + static_cast<float>(j) * cell,
						cell,
						shown
					);
	endMarks();
	glEnable(GL_LINE_SMOOTH);

	// "You are here": a red ring round the archaeologist.
	float px, py;
	Game().dungeon.getC(px, py);
	sk.loop(px, py + 0.5f, 0.6f, 0.6f, 9, PENCIL_RED, 1.f, 7u);
	sk.flush(2.f * pxScale);
	diamond(originX + px * cell, originY + (py + 0.5f) * cell, cell * 0.22f, PENCIL_RED, 1.f);
	glDisable(GL_LINE_SMOOTH);
	unclip();
	drawLegend(slip, rows, rowH, resScale); // over the sketch, where it lies over the sheet

	std::string title = "Level " + std::to_string(Game().dungeon.LevelNumber());
	if (!Game().dungeon.LevelTitle().empty())
		title += ": " + Game().dungeon.LevelTitle();
	beginText();
	textCentered(Game().assets.fonts.status, paper.cx(), paper.y + paper.h - PAPER_PAD_TOP + 2.5f, title.c_str(), INK);
	textCentered(
		small,
		paper.cx(),
		paper.y + 2.f,
		"Wheel, + / -: zoom    Drag, arrows: pan    M / Esc: close",
		INK_FADED
	);
	ScreenTabs::Draw();

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
}

void DraftMap::toCanvas(int x, int y, float& cx, float& cy) const {
	ui::toCanvas({0, 0, canvasW, CANVAS_H}, Game().render.resX, Game().render.resY, x, y, cx, cy);
}

void DraftMap::KeyPressed(unsigned char key) {
	if (fit.cell <= 0.f) // not drawn yet
		return;
	if (key == '+' || key == '=')
		zoom.ZoomAt(fit, MapZoom::STEP, fit.area.cx(), fit.area.cy());
	else if (key == '-' || key == '_')
		zoom.ZoomAt(fit, 1.f / MapZoom::STEP, fit.area.cx(), fit.area.cy());
	else
		pan(zoom, fit, navKeyOf(InputKey::Char(key), Game().settings.controls));
}

void DraftMap::SpecialKeyPressed(int key) {
	if (fit.cell > 0.f)
		pan(zoom, fit, navKeyOf(InputKey::Special(key), Game().settings.controls));
}

void DraftMap::MouseFunction(int button, int state, int x, int y) {
	if (fit.cell <= 0.f)
		return;
	float cx, cy;
	toCanvas(x, y, cx, cy);
	if (button == MOUSE_WHEEL_UP || button == MOUSE_WHEEL_DOWN) {
		if (state != GLUT_DOWN)
			return;
		if (!fit.area.contains(cx, cy)) { // off the sketch: round its centre
			cx = fit.area.cx();
			cy = fit.area.cy();
		}
		zoom.ZoomAt(fit, button == MOUSE_WHEEL_UP ? MapZoom::STEP : 1.f / MapZoom::STEP, cx, cy);
	} else if (button == MOUSE_LEFT_BUTTON) {
		dragging = state == GLUT_DOWN;
		dragX = cx;
		dragY = cy;
	}
}

void DraftMap::MouseMotion(int x, int y) {
	if (!dragging || fit.cell <= 0.f)
		return;
	float cx, cy;
	toCanvas(x, y, cx, cy);
	zoom.Pan(fit, cx - dragX, cy - dragY);
	dragX = cx;
	dragY = cy;
}
