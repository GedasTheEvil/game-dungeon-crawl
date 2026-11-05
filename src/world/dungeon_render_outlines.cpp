// The skills' outlines (plan heka-surplus, the rules in dungeon_seshat.cpp): the Wisdom of Seshat's faint pale frames
// round the traps near the player (on the floor, the back wall or the ceiling), Disarm trap's red ones round the
// trapped containers and the disguised mimics. Additive and unlit: they show in the dark. drawMechanismEffects' frame.
#include "dungeon.h"
#include "../entities/item.h"
#include "../state/assets.h"
#include "../graphics/lighting.h"
#include "../graphics/render_config.h"
#include <GL/gl.h>

namespace {
struct Vec {
	float x, y, z;
};
[[nodiscard]] Vec operator+(Vec a, Vec b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
[[nodiscard]] Vec operator*(Vec a, float k) { return {a.x * k, a.y * k, a.z * k}; }

constexpr float TS = RenderConfig::TILE_SIZE;
constexpr float BAND = 2.2f; // world units: the glow's half width on each side of its line
constexpr float MARGIN = 3.f; // the frame's line this far inside the cell
constexpr float SURFACE_LIFT = 0.6f; // off the floor, the wall or the ceiling, so it is not hidden in it
constexpr float SESHAT_ALPHA = 0.4f;
constexpr float DISARM_ALPHA = 0.55f;
constexpr float SESHAT_COLOUR[3] = {0.55f, 0.8f, 1.f}; // pale blue, faint
constexpr float DISARM_COLOUR[3] = {1.f, 0.18f, 0.1f};
// The frame round a container: its model's largest side (ContainerDef::scale) times these, a little in front of it.
constexpr float CONTAINER_WIDTH = 1.5f;
constexpr float CONTAINER_HEIGHT = 1.1f;
constexpr float CONTAINER_FRONT = 0.6f;
constexpr float SMALL_BAND = 1.2f; // the glow's half width round a container, smaller than a cell's

// A soft glowing frame in the plane through `corner` along the axes u and v (unit vectors), w x h world units: a band
// bright on its line, fading to nothing BAND inside and outside it.
void softFrame(Vec corner, Vec u, Vec v, float w, float h, const float (&colour)[3], float alpha, float band = BAND) {
	auto at = [&](float a, float b) { return corner + u * a + v * b; };
	auto vertex = [&](float a, float b, float strength) {
		glColor4f(colour[0], colour[1], colour[2], alpha * strength);
		const Vec p = at(a, b);
		glVertex3f(p.x, p.y, p.z);
	};
	// The outer, line and inner rectangles' corners, counter-clockwise from the bottom left.
	const float ring[3][4][2] = {
		{{-band, -band}, {w + band, -band}, {w + band, h + band}, {-band, h + band}},
		{{0, 0}, {w, 0}, {w, h}, {0, h}},
		{{band, band}, {w - band, band}, {w - band, h - band}, {band, h - band}},
	};
	constexpr float STRENGTH[3] = {0.f, 1.f, 0.f};
	for (int band = 0; band < 2; band++) {
		glBegin(GL_QUAD_STRIP);
		for (int k = 0; k <= 4; k++) {
			const int c = k % 4;
			vertex(ring[band][c][0], ring[band][c][1], STRENGTH[band]);
			vertex(ring[band + 1][c][0], ring[band + 1][c][1], STRENGTH[band + 1]);
		}
		glEnd();
	}
}
} // namespace

void Dungeon::drawOutlines(const CellRect& drawn) {
	const int col0 = view().originCol;
	const int row0 = view().originRow;
	bool begun = false;
	auto begin = [&]() {
		if (begun)
			return;
		begun = true;
		sim.assets->textures.nullTex.Bind();
		Lighting::setEmissive(true);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glDepthMask(GL_FALSE);
	};
	const Vec right{1, 0, 0}, up{0, 1, 0}, back{0, 0, -1};
	for (int j = drawn.row0; j < drawn.row0 + drawn.rows; j++)
		for (int i = drawn.col0; i < drawn.col0 + drawn.cols; i++) {
			if (!IsInBounds(i, j))
				continue;
			const int cell = MapIndex(i, j);
			const float x = static_cast<float>(i - col0) * TS;
			const float y = static_cast<float>(j - row0) * TS;
			if (const float near = trapOutlineAt(cell); near > 0.f) {
				begin();
				const Tile& t = map[cell];
				const float w = TS - 2 * MARGIN;
				const float alpha = SESHAT_ALPHA * near;
				switch (trapOutline(t)) {
				case TrapOutline::Floor: {
					const float floor = (hasShape(t) ? floorHeight(t, 0.5f) * TS : 0.f) + SURFACE_LIFT;
					softFrame({x + MARGIN, y + floor, -MARGIN}, right, back, w, w, SESHAT_COLOUR, alpha);
					break;
				}
				case TrapOutline::Wall:
					softFrame({x + MARGIN, y + MARGIN, -TS + SURFACE_LIFT}, right, up, w, w, SESHAT_COLOUR, alpha);
					break;
				case TrapOutline::Ceiling: {
					const float ceiling = (hasShape(t) ? ceilingHeight(t, 0.5f) * TS : TS) - SURFACE_LIFT;
					softFrame({x + MARGIN, y + ceiling, -MARGIN}, right, back, w, w, SESHAT_COLOUR, alpha);
					break;
				}
				case TrapOutline::None:
					break;
				}
			}
			if (const float near = containerOutlineAt(cell); near > 0.f) {
				begin();
				const TreasureLook look = treasureLook(cell);
				const float size = containerDef(look.kind).scale;
				const float w = size * CONTAINER_WIDTH;
				softFrame(
					{x + RenderConfig::ITEM_OFFSET_X + look.x * TS - w / 2,
					 y + SURFACE_LIFT,
					 RenderConfig::ITEM_OFFSET_Z - Item::DRAW_DEPTH + look.z * TS + size * CONTAINER_FRONT},
					right,
					up,
					w,
					size * CONTAINER_HEIGHT,
					DISARM_COLOUR,
					DISARM_ALPHA * near,
					SMALL_BAND
				);
			}
		}
	for (const Monster& mon : monsters)
		if (const float near = mimicOutline(mon); near > 0.f) {
			begin();
			softFrame(
				{(mon.Left() - static_cast<float>(col0)) * TS,
				 (mon.BottomY() - static_cast<float>(row0)) * TS + SURFACE_LIFT,
				 RenderConfig::MONSTER_DEPTH + mon.HalfWidth() * TS},
				right,
				up,
				(mon.Right() - mon.Left()) * TS,
				(mon.TopY() - mon.BottomY()) * TS,
				DISARM_COLOUR,
				DISARM_ALPHA * near,
				SMALL_BAND
			);
		}
	if (!begun)
		return;
	glDepthMask(GL_TRUE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	Lighting::setEmissive(false);
	glColor3f(1, 1, 1);
}
