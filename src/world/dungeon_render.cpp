#include "dungeon.h"
#include "../state/game_state.h"
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
	DecorSet& tex = Game().assets.decor;

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
	tex.wallTex[cell.wall].Bind();
	{
		const float n[3] = {0, 0, 1};
		const float v[4][3] = {{0, 0, -T}, {T, 0, -T}, {T, T, -T}, {0, T, -T}};
		const float st[4][2] = {{w0, 0}, {w1, 0}, {w1, 1}, {w0, 1}};
		quad(n, v, st);
	}
	if (isRock(i - 1, j)) {
		const float n[3] = {1, 0, 0};
		const float v[4][3] = {{0, 0, -T}, {0, 0, 0}, {0, T, 0}, {0, T, -T}};
		const float st[4][2] = {{w0, 0}, {w1, 0}, {w1, 1}, {w0, 1}};
		quad(n, v, st);
	}
	if (isRock(i + 1, j)) {
		const float n[3] = {-1, 0, 0};
		const float v[4][3] = {{T, 0, -T}, {T, 0, 0}, {T, T, 0}, {T, T, -T}};
		const float st[4][2] = {{w0, 0}, {w1, 0}, {w1, 1}, {w0, 1}};
		quad(n, v, st);
	}
	if (isRock(i, j - 1)) {
		const float n[3] = {0, 1, 0};
		const float v[4][3] = {{0, 0, 0}, {T, 0, 0}, {T, 0, -T}, {0, 0, -T}};
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
	Game().assets.items.chest->Draw();

	// The item turns over the chest (treasureSpin). The prototype is shared: its angle and size are only borrowed.
	if (std::optional<ItemKind> kind = itemFromFile(tile.attr, tile.value)) {
		Item* item = Game().assets.items.Of(*kind);
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

	Trap* tileTrap = isDeathTrap ? Game().assets.traps.deathTrap.get() : Game().assets.traps.spikes.get();
	tileTrap->Show();

	glPopMatrix();
}
//======================================================================================
void Dungeon::drawPortal(const float normal[3], const float v[4][3]) const {
	float px = static_cast<float>((static_cast<int>(portalScroll * 100) % 100)) / 200.0f;
	const float st[4][2] = {{px, 0}, {px, 1}, {px + 1, 1}, {px + 1, 0}};
	Game().assets.textures.portal.Bind();
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
	Game().assets.textures.columns.Bind();
	Game().assets.models.columns->Show();
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
	Game().assets.textures.ankh.Bind();
	Game().assets.models.ankh->Show();
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
	Game().assets.textures.sphinx.Bind();
	Game().assets.models.sphinx->Show();
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
		Game().assets.textures.questionMark.Bind();
		glRotatef(riddleMarkYaw, 0, 1, 0);
		Game().assets.models.question->Show();
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
	case Wall:
	case Empty:
	case Area3D:
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
void Dungeon::Draw(const HitboxView* hitboxes) {
	glPushMatrix();
	glTranslatef(-RenderConfig::TILE_SIZE * (mapX - static_cast<float>(static_cast<int>(mapX))),
				 -RenderConfig::TILE_SIZE * (mapY - static_cast<float>(static_cast<int>(mapY))), 0.f);
	glTranslatef(RenderConfig::TILE_SIZE * 2, RenderConfig::TILE_RENDER_Y, 0);

	addLights();
	Lighting::commit();

	glPushMatrix();								  // the loop walks the frame from tile to tile
	glTranslatef(-RenderConfig::TILE_SIZE, 0, 0); // from the view's first column, left of the origin
	const ViewWindow v = view();
	for (int j = v.originRow; j < v.originRow + ViewWindow::HEIGHT; j++) {
		for (int i = v.firstCol(); i < v.firstCol() + ViewWindow::WIDTH; i++) {
			if (IsInBounds(i, j)) {
				drawCellSurfaces(i, j);
				drawDecalTile(i, j);
				drawDecorTile(i, j);
				drawTorchTile(i, j);
				drawTileContent(i, j);
			}
			glTranslatef(RenderConfig::TILE_SIZE, 0, 0);
		}
		glTranslatef(-RenderConfig::TILE_SIZE * ViewWindow::WIDTH, RenderConfig::TILE_SIZE, 0); // the next row's start
	}
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-RenderConfig::TILE_SIZE, 0, 0);
	DrawMonsters();
	drawArrows();
	if (hitboxes != nullptr)
		drawHitboxes(*hitboxes);
	glPopMatrix();

	drawFires();
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
	Game().assets.textures.nullTex.Bind();
	Lighting::setEmissive(true);
	glDisable(GL_DEPTH_TEST);
	glLineWidth(2.f);
	glColor3f(1, 0.2f, 0.2f);
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Alive())
			box(mon.Left(), mon.Right(), mon.BottomY(), mon.TopY());
	const Player& player = *Game().player;
	const float half = player.HalfWidth();
	glColor3f(0.2f, 1, 0.2f);
	box(mapX - half, mapX + half, mapY, mapY + player.Height());
	const auto dir = static_cast<float>(Game().camera.Facing());
	const float from = weapon.fromEdge ? half : 0.f;
	glColor3f(1, 1, 0.2f);
	box(mapX + dir * from, mapX + dir * (from + weapon.reach), mapY, mapY + player.Height() / 2.f);
	glLineWidth(1.f);
	glEnable(GL_DEPTH_TEST);
	Lighting::setEmissive(false);
	glColor3f(1, 1, 1);
}
