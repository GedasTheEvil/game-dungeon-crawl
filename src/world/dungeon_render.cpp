#include "dungeon.h"
#include "../state/assets.h"
#include "../entities/player.h"
#include "item_bag.h"
#include "journal.h"
#include "rng.h"
#include "world_events.h"
#include <GL/gl.h>
#include <cmath>
#include "../graphics/render_config.h"
#include "../graphics/lighting.h"
#include "tile_defs.h"

namespace {
constexpr float CHEST_CLUB_SCALE = 10.f; // the club is small next to the chest at its own scale

// One face of a cell, corners counter-clockwise from (s0, t0).
void quad(const float n[3], const float v[4][3], const float st[4][2]) {
	glBegin(GL_QUADS);
	glNormal3fv(n);
	for (int k = 0; k < 4; k++) {
		glTexCoord2fv(st[k]);
		glVertex3fv(v[k]);
	}
	glEnd();
}
} // namespace

// Solid cells show the rock face, one texture over 2 x 2 cells. An open cell is a box: its back wall, the side walls
// against solid neighbours, the floor over a solid cell and the ceiling under one. On the side walls u runs from the
// back to the front, so they meet the back wall's edge seamlessly; on the floor and ceiling t = 1 is the back edge.
void Dungeon::drawCellSurfaces(int i, int j) {
	constexpr float T = RenderConfig::TILE_SIZE;
	DecorSet& tex = sim.assets->decor;

	if (isRock(i, j)) {
		float u0 = static_cast<float>(i & 1) * 0.5f;
		float t0 = static_cast<float>(j & 1) * 0.5f;
		const float n[3] = {0, 0, 1};
		const float v[4][3] = {{0, 0, 0}, {T, 0, 0}, {T, T, 0}, {0, T, 0}};
		const float st[4][2] = {{u0, t0}, {u0 + 0.5f, t0}, {u0 + 0.5f, t0 + 0.5f}, {u0, t0 + 0.5f}};
		glDisable(GL_BLEND);
		tex.rockTex.Bind();
		quad(n, v, st);
		return;
	}

	const SurfaceCell& cell = surface[MapIndex(i, j)];
	float w0 = cell.wallMirror ? 1.f : 0.f;
	float w1 = 1.f - w0;
	// Half water lies in a basin: its walls reach down to the basin floor, a stone wall faces a dry neighbour.
	const bool basin = IsInBounds(i, j) && inHalfWater(MapAt(i, j));
	const float b = basin ? -RenderConfig::WATER_BASIN_DEPTH : 0.f;
	const float tb = b / T; // the wall texture runs on below the row
	// Under a basin the walls stop at its floor, where the basin's own walls take over.
	const float h = IsInBounds(i, j + 1) && inHalfWater(MapAt(i, j + 1)) ? T - RenderConfig::WATER_BASIN_DEPTH : T;
	tex.wallTex[cell.wall].Bind();
	{
		const float n[3] = {0, 0, 1};
		const float v[4][3] = {{0, b, -T}, {T, b, -T}, {T, h, -T}, {0, h, -T}};
		const float st[4][2] = {{w0, tb}, {w1, tb}, {w1, h / T}, {w0, h / T}};
		quad(n, v, st);
	}
	if (isRock(i - 1, j) || (basin && dryOpen(i - 1, j))) {
		const float top = isRock(i - 1, j) ? h : 0.f;
		const float n[3] = {1, 0, 0};
		const float v[4][3] = {{0, b, -T}, {0, b, 0}, {0, top, 0}, {0, top, -T}};
		const float st[4][2] = {{w0, tb}, {w1, tb}, {w1, top / T}, {w0, top / T}};
		quad(n, v, st);
	}
	if (isRock(i + 1, j) || (basin && dryOpen(i + 1, j))) {
		const float top = isRock(i + 1, j) ? h : 0.f;
		const float n[3] = {-1, 0, 0};
		const float v[4][3] = {{T, b, -T}, {T, b, 0}, {T, top, 0}, {T, top, -T}};
		const float st[4][2] = {{w0, tb}, {w1, tb}, {w1, top / T}, {w0, top / T}};
		quad(n, v, st);
	}
	if (isRock(i, j - 1)) {
		const float n[3] = {0, 1, 0};
		const float v[4][3] = {{0, b, 0}, {T, b, 0}, {T, b, -T}, {0, b, -T}};
		const float st[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
		tex.floorTex[cell.floor].Bind();
		quad(n, v, st);
	}
	if (isRock(i, j + 1)) {
		const float n[3] = {0, -1, 0};
		const float v[4][3] = {{0, T, 0}, {T, T, 0}, {T, T, -T}, {0, T, -T}};
		const float st[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
		tex.ceilingTex[cell.ceiling].Bind();
		quad(n, v, st);
	}
	glColor3f(1, 1, 1);
}
//======================================================================================
void Dungeon::DrawTreasureTile(int i, int j) {
	const Tile tile = MapAt(i, j);

	glPushMatrix();
	glTranslatef(RenderConfig::ITEM_OFFSET_X, 0, RenderConfig::ITEM_OFFSET_Z);
	sim.assets->items.chest->Draw();

	// The item turns over the chest (treasureSpin). The prototype is shared: its angle and size are only borrowed.
	if (std::optional<ItemKind> kind = itemFromFile(tile.attr, tile.value)) {
		Item* item = sim.assets->items.Of(*kind);
		const float angle = item->rotA;
		const float scale = item->scale;
		item->rotA = treasureSpin;
		if (*kind == ItemKind::Club)
			item->scale = CHEST_CLUB_SCALE;
		item->Draw();
		item->rotA = angle;
		item->scale = scale;
	}

	glPopMatrix();
}
//======================================================================================
void Dungeon::DrawTrapTile(bool isDeathTrap) {
	glPushMatrix();
	glTranslatef(RenderConfig::ITEM_OFFSET_X, 0, RenderConfig::ITEM_OFFSET_Z);

	Trap* tileTrap = isDeathTrap ? sim.assets->traps.deathTrap.get() : sim.assets->traps.spikes.get();
	tileTrap->Show();

	glPopMatrix();
}
//======================================================================================
void Dungeon::drawPortal(const float normal[3], const float v[4][3]) const {
	float px = static_cast<float>((static_cast<int>(portalScroll * 100) % 100)) / 200.0f;
	const float st[4][2] = {{px, 0}, {px, 1}, {px + 1, 1}, {px + 1, 0}};
	sim.assets->textures.portal.Bind();
	Lighting::setEmissive(true);
	glBegin(GL_QUADS);
	glNormal3fv(normal);
	for (int k = 0; k < 4; k++) {
		glTexCoord2fv(st[k]);
		glVertex3fv(v[k]);
	}
	glEnd();
	Lighting::setEmissive(false);
}
//======================================================================================
// The plasma fills the gap between the columns, in their centre plane (props.py, build_columns).
void Dungeon::drawTeleporterTile() {
	glPushMatrix();
	glTranslatef(20, 0, -30);
	glScalef(40, 40, 40);
	sim.assets->textures.columns.Bind();
	sim.assets->models.columns->Show();
	glPopMatrix();

	const float normal[3] = {0, 0, 1};
	const float v[4][3] = {{10, 0, -30}, {10, 36, -30}, {30, 36, -30}, {30, 0, -30}};
	drawPortal(normal, v);
}
//======================================================================================
void Dungeon::drawAnkhTile() {
	glPushMatrix();
	glTranslatef(RenderConfig::TILE_HALF, 0, -RenderConfig::TILE_HALF); // the cell centre, halfway to the back wall
	glScalef(40, 40, 40);
	sim.assets->textures.ankh.Bind();
	sim.assets->models.ankh->Show();
	glPopMatrix();
}
//======================================================================================
// The doorway stands against the side wall, plasma inside it; with no side wall it turns to the back wall and faces
// the camera. Unturned it is on the left.
void Dungeon::drawDoorTile(int i, int j) {
	const Tile tile = MapAt(i, j);
	float yaw = 0;
	if (isRock(i + 1, j) && !isRock(i - 1, j))
		yaw = 180;
	else if (!isRock(i + 1, j) && !isRock(i - 1, j))
		yaw = -90;

	glPushMatrix();
	glTranslatef(RenderConfig::TILE_HALF, 0, -RenderConfig::TILE_HALF); // the cell centre, halfway to the back wall
	glPushMatrix();
	glRotatef(yaw, 0, 1, 0);
	glPushMatrix();
	glScalef(40, 40, 40);
	sim.assets->textures.sphinx.Bind();
	sim.assets->models.sphinx->Show();
	glPopMatrix();

	if (tile.attr == GateEntrance || tile.attr == GateExit) {
		const float normal[3] = {1, 0, 0};
		const float v[4][3] = {{-19.6f, 0, -10}, {-19.6f, 35, -10}, {-19.6f, 35, 10}, {-19.6f, 0, 10}};
		drawPortal(normal, v);
	}
	glPopMatrix();

	if (tile.attr == GateRiddle) {
		glPushMatrix();
		glTranslatef(0, 20, 0);
		glScalef(10, 10, 10);
		sim.assets->textures.questionMark.Bind();
		glRotatef(riddleMarkYaw, 0, 1, 0);
		sim.assets->models.question->Show();
		glPopMatrix();
	}
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawTileContent(int i, int j) {
	const Tile tile = MapAt(i, j);
	if (!isTileType(tile.type))
		return; // the game treats an unknown type as open space
	switch (static_cast<DungeonTileType>(tile.type)) {
	case NoObject:
	case MonsterSpawn: // the monster is drawn where it is now (DrawMonsters)
		break;
	case Treasure:
		DrawTreasureTile(i, j);
		break;
	case Spike:
		DrawTrapTile(false);
		break;
	case Death:
		DrawTrapTile(true);
		break;
	case Ankh:
		drawAnkhTile();
		break;
	case Door:
		if (isTeleporter(tile))
			drawTeleporterTile();
		else
			drawDoorTile(i, j);
		break;
	case Ladder:
		drawLadderTile(i, j);
		break;
	case Key:
		drawKeyTile(i, j);
		break;
	case Gate:
		drawGateTile(i, j);
		break;
	case Lever:
		drawLeverTile(i, j);
		break;
	case RockFall:
		drawRockFallTile(i, j);
		break;
	}
}
//======================================================================================
void Dungeon::pushLevelFrame() const {
	glPushMatrix();
	glTranslatef(-RenderConfig::TILE_SIZE * (mapX - static_cast<float>(static_cast<int>(mapX))),
				 -RenderConfig::TILE_SIZE * (mapY - static_cast<float>(static_cast<int>(mapY))), 0.f);
	glTranslatef(RenderConfig::TILE_SIZE * 2, RenderConfig::TILE_RENDER_Y, 0);
}
//======================================================================================
void Dungeon::Draw(const HitboxView* hitboxes) {
	pushLevelFrame();

	const ViewWindow v = view();
	float projection[16];
	float modelview[16];
	glGetFloatv(GL_PROJECTION_MATRIX, projection);
	glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
	float clip[16];
	for (int c = 0; c < 4; c++)
		for (int r = 0; r < 4; r++) {
			clip[c * 4 + r] = 0.f;
			for (int k = 0; k < 4; k++)
				clip[c * 4 + r] += projection[k * 4 + r] * modelview[c * 4 + k];
		}
	const CellRect drawn = drawnWindow(v, clip, RenderConfig::TILE_SIZE);
	drawnCells = drawn;

	addLights(drawn);
	Lighting::commit();

	glPushMatrix(); // the loop walks the frame from tile to tile, from the drawn window's first cell
	glTranslatef(RenderConfig::TILE_SIZE * static_cast<float>(drawn.col0 - v.originCol),
				 RenderConfig::TILE_SIZE * static_cast<float>(drawn.row0 - v.originRow), 0);
	for (int j = drawn.row0; j < drawn.row0 + drawn.rows; j++) {
		for (int i = drawn.col0; i < drawn.col0 + drawn.cols; i++) {
			drawCellSurfaces(i, j); // outside the level: rock
			if (IsInBounds(i, j)) {
				drawDecalTile(i, j);
				drawDecorTile(i, j);
				drawTorchTile(i, j);
				// What stands in half water stands on the basin floor; a ladder runs on up out of it.
				const bool sunk = inHalfWater(MapAt(i, j)) && MapAt(i, j).type != Ladder;
				glPushMatrix();
				glTranslatef(0, sunk ? -RenderConfig::WATER_BASIN_DEPTH : 0.f, 0);
				drawTileContent(i, j);
				glPopMatrix();
			}
			glTranslatef(RenderConfig::TILE_SIZE, 0, 0);
		}
		glTranslatef(-RenderConfig::TILE_SIZE * static_cast<float>(drawn.cols), RenderConfig::TILE_SIZE,
					 0); // the next row's start
	}
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-RenderConfig::TILE_SIZE, 0, 0);
	DrawMonsters(drawn);
	drawArrows();
	if (hitboxes != nullptr)
		drawHitboxes(*hitboxes);
	glPopMatrix();

	drawFires(drawn);
	drawMechanismEffects();
	drawSummonEffects();
	glPopMatrix();
}
//======================================================================================
// Debug view (RenderSettings::Hitboxes), in DrawMonsters' frame: the monster boxes red, the player's green, the reach
// of the equipped weapon yellow.
void Dungeon::drawHitboxes(const HitboxView& weapon) {
	const auto firstCol = static_cast<float>(view().firstCol());
	const auto firstRow = static_cast<float>(view().originRow);
	constexpr float DEPTH = RenderConfig::MONSTER_DEPTH;
	auto box = [&](float left, float right, float bottom, float top) {
		glBegin(GL_LINE_LOOP);
		glVertex3f(RenderConfig::TILE_SIZE * (left - firstCol), RenderConfig::TILE_SIZE * (bottom - firstRow), DEPTH);
		glVertex3f(RenderConfig::TILE_SIZE * (right - firstCol), RenderConfig::TILE_SIZE * (bottom - firstRow), DEPTH);
		glVertex3f(RenderConfig::TILE_SIZE * (right - firstCol), RenderConfig::TILE_SIZE * (top - firstRow), DEPTH);
		glVertex3f(RenderConfig::TILE_SIZE * (left - firstCol), RenderConfig::TILE_SIZE * (top - firstRow), DEPTH);
		glEnd();
	};
	sim.assets->textures.nullTex.Bind();
	Lighting::setEmissive(true);
	glDisable(GL_DEPTH_TEST);
	glLineWidth(2.f);
	glColor3f(1, 0.2f, 0.2f);
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Alive())
			box(mon.Left(), mon.Right(), mon.BottomY(), mon.TopY());
	const Player& player = *sim.player;
	const float half = player.HalfWidth();
	glColor3f(0.2f, 1, 0.2f);
	box(mapX - half, mapX + half, mapY, mapY + player.Height());
	const auto dir = static_cast<float>(weapon.facing);
	const float from = weapon.fromEdge ? half : 0.f;
	glColor3f(1, 1, 0.2f);
	box(mapX + dir * from, mapX + dir * (from + weapon.reach), mapY, mapY + player.Height() / 2.f);
	glLineWidth(1.f);
	glEnable(GL_DEPTH_TEST);
	Lighting::setEmissive(false);
	glColor3f(1, 1, 1);
}
//======================================================================================
namespace {
struct Rgba {
	float r, g, b, a;
};
constexpr Rgba WATER_FRONT = {0.08f, 0.30f, 0.36f, 0.42f}; // the side of the water facing the camera
constexpr Rgba WATER_TOP = {0.20f, 0.48f, 0.52f, 0.50f};
constexpr Rgba WATER_GLINT = {0.80f, 0.90f, 0.85f, 0.25f}; // ripples on the surface, at most
constexpr Rgba WATERLINE = {0.75f, 0.90f, 0.88f, 0.55f};   // the surface's front edge
constexpr Rgba DEEP_FRONT = {0.03f, 0.10f, 0.15f, 0.80f};
constexpr int RIPPLE_STRIPS = 6; // across the surface, front to back
constexpr float RIPPLE_SPEED = 1.3f;
constexpr float WATERLINE_HEIGHT = 0.4f; // world units

void colour(const Rgba& c, float alpha) { glColor4f(c.r, c.g, c.b, c.a * alpha); }
} // namespace

// Half water fills its basin (WATER_BASIN_DEPTH under the row's floor) up to WATER_SURFACE: a surface with drifting
// glints; over deep water also a see-through front with a bright waterline (a rock cell below hides the basin's
// front). Deep water: a dark front up to the basin of the half water above it.
void Dungeon::drawWaterCell(int i, int j, float x, float y) {
	constexpr float T = RenderConfig::TILE_SIZE;
	constexpr float B = RenderConfig::WATER_BASIN_DEPTH;
	const Structure s = MapAt(i, j).structure;
	glBegin(GL_QUADS);
	if (s == Structure::DeepWater) {
		const float top = inHalfWater(MapAt(i, j + 1)) ? T - B : T;
		glNormal3f(0, 0, 1);
		colour(DEEP_FRONT, 1.f);
		glVertex3f(x, y, 0);
		glVertex3f(x + T, y, 0);
		glVertex3f(x + T, y + top, 0);
		glVertex3f(x, y + top, 0);
		glEnd();
		return;
	}
	constexpr float S = RenderConfig::WATER_SURFACE;
	if (MapAt(i, j - 1).structure == Structure::DeepWater) {
		glNormal3f(0, 0, 1);
		colour(WATER_FRONT, 1.f);
		glVertex3f(x, y - B, 0);
		glVertex3f(x + T, y - B, 0);
		glVertex3f(x + T, y + S, 0);
		glVertex3f(x, y + S, 0);
		colour(WATERLINE, 1.f);
		glVertex3f(x, y + S - WATERLINE_HEIGHT, 0.01f);
		glVertex3f(x + T, y + S - WATERLINE_HEIGHT, 0.01f);
		glVertex3f(x + T, y + S, 0.01f);
		glVertex3f(x, y + S, 0.01f);
	}

	glNormal3f(0, 1, 0);
	colour(WATER_TOP, 1.f);
	glVertex3f(x, y + S, 0);
	glVertex3f(x, y + S, -T);
	glVertex3f(x + T, y + S, -T);
	glVertex3f(x + T, y + S, 0);
	// Glints drifting along the row, a little above the surface so they do not fight it.
	const float time = static_cast<float>(GameClock::now()) / 1000.f * RIPPLE_SPEED;
	for (int k = 0; k < RIPPLE_STRIPS; k++) {
		const float z0 = -T * static_cast<float>(k) / RIPPLE_STRIPS;
		const float z1 = -T * static_cast<float>(k + 1) / RIPPLE_STRIPS;
		const auto phase = static_cast<float>(i) * 1.7f + static_cast<float>(k) * 2.3f;
		const float left = 0.5f + 0.5f * std::sin(time + phase);
		const float right = 0.5f + 0.5f * std::sin(time + phase + 1.9f);
		colour(WATER_GLINT, left * left);
		glVertex3f(x, y + S + 0.05f, z0);
		glVertex3f(x, y + S + 0.05f, z1);
		colour(WATER_GLINT, right * right);
		glVertex3f(x + T, y + S + 0.05f, z1);
		glVertex3f(x + T, y + S + 0.05f, z0);
	}
	glEnd();
}
//======================================================================================
void Dungeon::DrawWater() {
	pushLevelFrame();
	const ViewWindow v = view();
	const CellRect& drawn = drawnCells;
	sim.assets->textures.nullTex.Bind();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE); // see-through: it hides nothing behind it, and the ink draws no lines on it
	for (int j = drawn.row0; j < drawn.row0 + drawn.rows; j++)
		for (int i = drawn.col0; i < drawn.col0 + drawn.cols; i++) {
			const Structure s = IsInBounds(i, j) ? MapAt(i, j).structure : Structure::Wall;
			if (s != Structure::HalfWater && s != Structure::DeepWater)
				continue;
			drawWaterCell(i, j, RenderConfig::TILE_SIZE * static_cast<float>(i - v.originCol),
						  RenderConfig::TILE_SIZE * static_cast<float>(j - v.originRow));
		}
	glDepthMask(GL_TRUE);
	glColor4f(1, 1, 1, 1);
	glPopMatrix();
}
