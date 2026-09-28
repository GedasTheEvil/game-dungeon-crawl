#include "dungeon.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
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
	DecorSet& tex = GAME_STATE.decor;
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
	GAME_STATE.items.chest->Draw();

	if (tile.attr == 3) {
		GAME_STATE.items.potion->Draw();
		GAME_STATE.items.potion->rotA++;
	}

	if (tile.attr == 2) {
		GAME_STATE.items.bow->Draw();
		GAME_STATE.items.bow->rotA++;
	}

	if (tile.attr == 1) {
		if (tile.value == 0) {
			GAME_STATE.items.club->scale = 10;
			GAME_STATE.items.club->Draw();
			GAME_STATE.items.club->rotA++;
		}
		if (tile.value == 1) {
			GAME_STATE.items.sword->Draw();
			GAME_STATE.items.sword->rotA++;
		}
		if (tile.value == 2) {
			GAME_STATE.items.spear->Draw();
			GAME_STATE.items.spear->rotA++;
		}
	}

	glPopMatrix();
}
//======================================================================================
void Dungeon::DrawTrapTile(int i, int j, bool isDeathTrap) {
	glPushMatrix();
	glTranslatef(RenderConfig::ITEM_OFFSET_X, 0, RenderConfig::ITEM_OFFSET_Z);

	Trap* tileTrap = isDeathTrap ? GAME_STATE.traps.DeathTrap.get() : GAME_STATE.traps.TrapD.get();
	tileTrap->dungeonCamX = &mapX;
	tileTrap->dungeonCamY = &mapY;
	tileTrap->setCords(static_cast<float>(i), static_cast<float>(j));
	tileTrap->Show();

	glPopMatrix();
}
//======================================================================================
void Dungeon::Draw() {
	bool plasmaAni;
	plasmaAni = aniT->TimePassed();

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

				if (tile.type == Monster)
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
					GAME_STATE.textures.ankh_t.Bind();
					GAME_STATE.models.ankh->Show();
					glPopMatrix();
				}
				if (tile.type == Door) {
					glPushMatrix();
					glTranslatef(20, 0, -20);
					glPushMatrix();
					glScalef(40, 40, 40);
					if (tile.attr != GateEntrance)
						glRotatef(180, 0, 1, 0);
					GAME_STATE.textures.sphinx_t.Bind();
					GAME_STATE.models.sphinx->Show();
					glPopMatrix();
					glPopMatrix();

					if (tile.attr == GateRiddle) {
						glPushMatrix();
						glTranslatef(20, 20, -20);
						glPushMatrix();
						glScalef(10, 10, 10);
						GAME_STATE.textures.question_t.Bind();
						glPushMatrix();
						glRotatef(qRot, 0, 1, 0);
						GAME_STATE.models.question->Show();
						qRot += 1.0;
						glPopMatrix();
						glPopMatrix();
						glPopMatrix();
					}

					if (tile.attr == GateEntrance || tile.attr == GateExit) {
						glPushMatrix();
						if (tile.attr != GateEntrance)
							glTranslatef(39, 0, 0);

						float px = static_cast<float>((static_cast<int>(plasma * 100) % 100)) / 200.0f;

						GAME_STATE.textures.plasma_t.Bind();
						Lighting::setEmissive(true);
						glBegin(GL_QUADS);
						glNormal3f(1, 0, 0);
						glTexCoord2f(px, 0);
						glVertex3f(0.4, 0, -30);
						glTexCoord2f(px, 1);
						glVertex3f(0.4, 35, -30);
						glTexCoord2f(px + 1, 1);
						glVertex3f(0.4, 35, -10);
						glTexCoord2f(px + 1, 0);
						glVertex3f(0.4, 0, -10);
						glEnd();
						Lighting::setEmissive(false);
						if (plasmaAni)
							plasma -= 0.022;
						glPopMatrix();
					}
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
	glPopMatrix();

	drawFires();
	drawMechanismEffects();
	glPopMatrix();
}
