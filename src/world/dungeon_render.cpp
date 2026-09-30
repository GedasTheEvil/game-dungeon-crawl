#include "dungeon.h"
#include "../state/game_state.h"
#include <GL/gl.h>
#include <cmath>
#include "../graphics/render_config.h"
#include "../graphics/lighting.h"

namespace {
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
	auto rock = [this](int col, int row) { return !IsInBounds(col, row) || MapAt(col, row).type == Wall; };

	if (rock(i, j)) {
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
	if (rock(i - 1, j)) {
		const float n[3] = {1, 0, 0};
		const float v[4][3] = {{0, 0, -T}, {0, 0, 0}, {0, T, 0}, {0, T, -T}};
		const float st[4][2] = {{w0, 0}, {w1, 0}, {w1, 1}, {w0, 1}};
		quad(n, v, st);
	}
	if (rock(i + 1, j)) {
		const float n[3] = {-1, 0, 0};
		const float v[4][3] = {{T, 0, -T}, {T, 0, 0}, {T, T, 0}, {T, T, -T}};
		const float st[4][2] = {{w0, 0}, {w1, 0}, {w1, 1}, {w0, 1}};
		quad(n, v, st);
	}
	if (rock(i, j - 1)) {
		const float n[3] = {0, 1, 0};
		const float v[4][3] = {{0, 0, 0}, {T, 0, 0}, {T, 0, -T}, {0, 0, -T}};
		const float st[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
		tex.floorTex[cell.floor].Bind();
		quad(n, v, st);
	}
	if (rock(i, j + 1)) {
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

	if (tile.attr == 3) {
		Game().assets.items.potion->Draw();
		Game().assets.items.potion->rotA++;
	}

	if (tile.attr == 2) {
		Game().assets.items.bow->Draw();
		Game().assets.items.bow->rotA++;
	}

	if (tile.attr == 1) {
		if (tile.value == 0) {
			Game().assets.items.club->scale = 10;
			Game().assets.items.club->Draw();
			Game().assets.items.club->rotA++;
		}
		if (tile.value == 1) {
			Game().assets.items.sword->Draw();
			Game().assets.items.sword->rotA++;
		}
		if (tile.value == 2) {
			Game().assets.items.spear->Draw();
			Game().assets.items.spear->rotA++;
		}
	}

	glPopMatrix();
}
//======================================================================================
void Dungeon::DrawTrapTile(int i, int j, bool isDeathTrap) {
	glPushMatrix();
	glTranslatef(RenderConfig::ITEM_OFFSET_X, 0, RenderConfig::ITEM_OFFSET_Z);

	Trap* tileTrap = isDeathTrap ? Game().assets.traps.deathTrap.get() : Game().assets.traps.spikes.get();
	tileTrap->dungeonCamX = &mapX;
	tileTrap->dungeonCamY = &mapY;
	tileTrap->setCords(static_cast<float>(i), static_cast<float>(j));
	tileTrap->Show();

	glPopMatrix();
}
//======================================================================================
void Dungeon::drawTeleporterTile() {
	glPushMatrix();
	glTranslatef(20, 0, -25);
	glScalef(40, 40, 40);
	Game().assets.textures.columns.Bind();
	Game().assets.models.columns->Show();
	glPopMatrix();

	float px = static_cast<float>((static_cast<int>(portalScroll * 100) % 100)) / 200.0f;
	Game().assets.textures.portal.Bind();
	Lighting::setEmissive(true);
	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glTexCoord2f(px, 0);
	glVertex3f(10, 0, -30);
	glTexCoord2f(px, 1);
	glVertex3f(10, 37, -30);
	glTexCoord2f(px + 1, 1);
	glVertex3f(30, 37, -30);
	glTexCoord2f(px + 1, 0);
	glVertex3f(30, 0, -30);
	glEnd();
	Lighting::setEmissive(false);
}
//======================================================================================
void Dungeon::Draw() {
	if (portalTimer.TimePassed()) // once per frame, however many portals are in view
		portalScroll -= 0.022;

	glPushMatrix();
	glTranslatef(-RenderConfig::TILE_SIZE * (mapX - static_cast<float>(static_cast<int>(mapX))),
				 -RenderConfig::TILE_SIZE * (mapY - static_cast<float>(static_cast<int>(mapY))), 0.f);
	glTranslatef(RenderConfig::TILE_SIZE * 2, RenderConfig::TILE_RENDER_Y, 0);

	addLights();
	Lighting::commit();

	glPushMatrix();								  // the loop walks the frame from tile to tile
	glTranslatef(-RenderConfig::TILE_SIZE, 0, 0); // one spare column each side of the (mapX - 3) origin
	for (int j = static_cast<int>(mapY) - 3; j < static_cast<int>(mapY) + 3; j++) {
		for (int i = static_cast<int>(mapX) - 4; i < static_cast<int>(mapX) + 6; i++) {
			if (IsInBounds(i, j)) {
				const Tile tile = MapAt(i, j);
				drawCellSurfaces(i, j);
				drawDecalTile(i, j);
				drawDecorTile(i, j);
				drawTorchTile(i, j);

				if (tile.type == MonsterSpawn)
					SpawnMonster(i, j);
				if (tile.type == Treasure)
					DrawTreasureTile(i, j);
				if (tile.type == Spike)
					DrawTrapTile(i, j, false);
				if (tile.type == Death)
					DrawTrapTile(i, j, true);
				if (tile.type == Ankh) {
					glPushMatrix();
					glTranslatef(20, 0, -20);
					glScalef(40, 40, 40);
					Game().assets.textures.ankh.Bind();
					Game().assets.models.ankh->Show();
					glPopMatrix();
				}
				if (isTeleporter(tile))
					drawTeleporterTile();
				else if (tile.type == Door) {
					// The doorway stands against the side wall, plasma inside it; with no side wall it turns to the
					// back wall and faces the camera. Unturned it is on the left.
					auto rock = [this](int col, int row) {
						return !IsInBounds(col, row) || MapAt(col, row).type == Wall;
					};
					float yaw = 0;
					if (rock(i + 1, j) && !rock(i - 1, j))
						yaw = 180;
					else if (!rock(i + 1, j) && !rock(i - 1, j))
						yaw = -90;

					glPushMatrix();
					glTranslatef(20, 0, -20);
					glPushMatrix();
					glRotatef(yaw, 0, 1, 0);
					glPushMatrix();
					glScalef(40, 40, 40);
					Game().assets.textures.sphinx.Bind();
					Game().assets.models.sphinx->Show();
					glPopMatrix();

					if (tile.attr == GateEntrance || tile.attr == GateExit) {
						float px = static_cast<float>((static_cast<int>(portalScroll * 100) % 100)) / 200.0f;

						Game().assets.textures.portal.Bind();
						Lighting::setEmissive(true);
						glBegin(GL_QUADS);
						glNormal3f(1, 0, 0);
						glTexCoord2f(px, 0);
						glVertex3f(-19.6, 0, -10);
						glTexCoord2f(px, 1);
						glVertex3f(-19.6, 35, -10);
						glTexCoord2f(px + 1, 1);
						glVertex3f(-19.6, 35, 10);
						glTexCoord2f(px + 1, 0);
						glVertex3f(-19.6, 0, 10);
						glEnd();
						Lighting::setEmissive(false);
					}
					glPopMatrix();

					if (tile.attr == GateRiddle) {
						glPushMatrix();
						glTranslatef(0, 20, 0);
						glScalef(10, 10, 10);
						Game().assets.textures.questionMark.Bind();
						glRotatef(riddleMarkYaw, 0, 1, 0);
						Game().assets.models.question->Show();
						riddleMarkYaw += 1.0;
						glPopMatrix();
					}
					glPopMatrix();
				}
				if (tile.type == Ladder)
					drawLadderTile(i, j);
				if (tile.type == Key)
					drawKeyTile(i, j);
				if (tile.type == Gate)
					drawGateTile(i, j);
				if (tile.type == Lever)
					drawLeverTile(i, j);
				if (tile.type == RockFall)
					drawRockFallTile(i, j);
			}
			glTranslatef(40, 0, 0);
		}
		glTranslatef(RenderConfig::HUD_OFFSET_X, RenderConfig::TILE_SIZE, 0);
	}
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-RenderConfig::TILE_SIZE, 0, 0);
	DrawMonsters();
	drawArrows();
	glPopMatrix();

	drawFires();
	drawMechanismEffects();
	glPopMatrix();
}
