#include "dungeon.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include <GL/gl.h>
#include <cmath>
#include "../graphics/render_config.h"

inline float dotProduct(VECTOR& v1, VECTOR& v2) { return v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z; }
void normalize(VECTOR& v);
void rotateVector(MATRIX& m, VECTOR& v, VECTOR& d);

void Dungeon::renderCartoonTile(int type, int left, int right, int up, int down) {
	if (type == Wall)
		return; // Wall tiles have no back-face geometry in cartoon mode

	MATRIX tmpMatrix;
	VECTOR tmpVector, tmpNormal;
	glGetFloatv(GL_MODELVIEW_MATRIX, tmpMatrix.Data);
	glEnable(GL_TEXTURE_1D);
	glBindTexture(GL_TEXTURE_1D, shaderTexture[0]);

	auto shade = [&](float nx, float ny, float nz) -> float {
		tmpNormal.X = nx;
		tmpNormal.Y = ny;
		tmpNormal.Z = nz;
		rotateVector(tmpMatrix, tmpNormal, tmpVector);
		normalize(tmpVector);
		float s = dotProduct(tmpVector, lightAngle);
		return s < 0.0f ? 0.0f : s;
	};

	// Back face with per-vertex normals for gradient shading
	glBegin(GL_QUADS);
	glTexCoord1f(shade(0, 0, 1));
	glVertex3i(0, 0, -40);
	glTexCoord1f(shade(0, 0.3f, 1));
	glVertex3i(0, 40, -40);
	glTexCoord1f(shade(0.2f, 0.3f, 1));
	glVertex3i(40, 40, -40);
	glTexCoord1f(shade(0.2f, 0.4f, 0.5f));
	glVertex3i(40, 0, -40);
	glEnd();

	auto renderWall = [&](float nx, float ny, float nz, int x1, int y1, int z1, int x2, int y2, int z2, int x3, int y3,
						  int z3, int x4, int y4, int z4) {
		float s = shade(nx, ny, nz);
		glBegin(GL_QUADS);
		glTexCoord1f(s);
		glVertex3i(x1, y1, z1);
		glVertex3i(x2, y2, z2);
		glVertex3i(x3, y3, z3);
		glVertex3i(x4, y4, z4);
		glEnd();
	};

	if (!left)
		renderWall(1, 0, 0, 0, 0, -40, 0, 40, -40, 0, 40, 0, 0, 0, 0);
	if (!right)
		renderWall(-1, 0, 0, 40, 0, -40, 40, 40, -40, 40, 40, 0, 40, 0, 0);
	if (!up)
		renderWall(0, -1, 0, 0, 40, -40, 40, 40, -40, 40, 40, 0, 0, 40, 0);
	if (!down)
		renderWall(0, 1, 0, 0, 0, -40, 40, 0, -40, 40, 0, 0, 0, 0, 0);

	glDisable(GL_TEXTURE_1D);
}
//======================================================================================
void Dungeon::renderFlatTile(int type, int left, int right, int up, int down) {
	if (GAME_STATE.render.Orig_model) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glColor4f(1, 1, 1, 0.4f);
	}

	if (type == Wall) {
		glDisable(GL_BLEND);
		GAME_STATE.textures.black_t.Bind();
		glBegin(GL_QUADS);
		glNormal3f(0, 0, 1);
		glTexCoord2f(0, 0);
		glVertex3i(0, 0, 0);
		glTexCoord2f(0, 1);
		glVertex3i(0, 40, 0);
		glTexCoord2f(1, 1);
		glVertex3i(40, 40, 0);
		glTexCoord2f(1, 0);
		glVertex3i(40, 0, 0);
		glEnd();
	} else {
		glBegin(GL_QUADS);
		glNormal3f(0, 0, 1);
		glTexCoord2f(0, 0);
		glVertex3i(0, 0, -40);
		glTexCoord2f(0, 1);
		glVertex3i(0, 40, -40);
		glTexCoord2f(1, 1);
		glVertex3i(40, 40, -40);
		glTexCoord2f(1, 0);
		glVertex3i(40, 0, -40);
		glEnd();

		if (!left) {
			glBegin(GL_QUADS);
			glNormal3f(1, 0, 0);
			glTexCoord2f(0, 0);
			glVertex3i(0, 0, -40);
			glTexCoord2f(0, 1);
			glVertex3i(0, 40, -40);
			glTexCoord2f(1, 1);
			glVertex3i(0, 40, 0);
			glTexCoord2f(1, 0);
			glVertex3i(0, 0, 0);
			glEnd();
		}
		if (!right) {
			glBegin(GL_QUADS);
			glNormal3f(-1, 0, 0);
			glTexCoord2f(0, 0);
			glVertex3i(40, 0, -40);
			glTexCoord2f(0, 1);
			glVertex3i(40, 40, -40);
			glTexCoord2f(1, 1);
			glVertex3i(40, 40, 0);
			glTexCoord2f(1, 0);
			glVertex3i(40, 0, 0);
			glEnd();
		}
		if (!up) {
			glBegin(GL_QUADS);
			glNormal3f(0, -1, 0);
			glTexCoord2f(0.3f, 0.3f);
			glVertex3i(0, 40, -40);
			glTexCoord2f(0.3f, 0.7f);
			glVertex3i(40, 40, -40);
			glTexCoord2f(0.7f, 0.7f);
			glVertex3i(40, 40, 0);
			glTexCoord2f(0.7f, 0.3f);
			glVertex3i(0, 40, 0);
			glEnd();
		}
		if (!down) {
			glBegin(GL_QUADS);
			glNormal3f(0, 1, 0);
			glTexCoord2f(0.3f, 0.3f);
			glVertex3i(0, 0, -40);
			glTexCoord2f(0.3f, 0.7f);
			glVertex3i(40, 0, -40);
			glTexCoord2f(0.7f, 0.7f);
			glVertex3i(40, 0, 0);
			glTexCoord2f(0.7f, 0.3f);
			glVertex3i(0, 0, 0);
			glEnd();
		}
	}

	if (GAME_STATE.render.Orig_model)
		glDisable(GL_BLEND);
}
//======================================================================================
void Dungeon::DrawSegment(int type, int leftWallType, int rightWallType, int upWallType, int downWallType) {
	GAME_STATE.textures.blackTex.Bind();
	if (GAME_STATE.render.Cartoon)
		renderCartoonTile(type, leftWallType, rightWallType, upWallType, downWallType);
	if (!GAME_STATE.render.Cartoon || GAME_STATE.render.Orig_model)
		renderFlatTile(type, leftWallType, rightWallType, upWallType, downWallType);
	glColor3f(1, 1, 1);
}
//======================================================================================
void Dungeon::DrawTreasureTile(int i, int j) {
	const Tint tile = MapAt(i, j);

	glPushMatrix();
	glTranslatef(RenderConfig::ITEM_OFFSET_X, 0, RenderConfig::ITEM_OFFSET_Z);
	GAME_STATE.items.chest->Draw();

	if (tile.b == 3) {
		GAME_STATE.items.potion->Draw();
		GAME_STATE.items.potion->rotA++;
	}

	if (tile.b == 2) {
		GAME_STATE.items.bow->Draw();
		GAME_STATE.items.bow->rotA++;
	}

	if (tile.b == 1) {
		if (tile.c == 0) {
			GAME_STATE.items.club->scale = 10;
			GAME_STATE.items.club->Draw();
			GAME_STATE.items.club->rotA++;
		}
		if (tile.c == 1) {
			GAME_STATE.items.sword->Draw();
			GAME_STATE.items.sword->rotA++;
		}
		if (tile.c == 2) {
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

	trap* tileTrap = isDeathTrap ? GAME_STATE.traps.DeathTrap.get() : GAME_STATE.traps.TrapD.get();
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

	for (int j = static_cast<int>(mapY) - 3; j < static_cast<int>(mapY) + 3; j++) {
		for (int i = static_cast<int>(mapX) - 3; i < static_cast<int>(mapX) + 5; i++) {
			if (IsInBounds(i, j)) {
				const Tint tile = MapAt(i, j);
				DrawSegment(tile.a, MapAt(i - 1, j).a, MapAt(i + 1, j).a, MapAt(i, j + 1).a, MapAt(i, j - 1).a);

				if (tile.a == Monster)
					DrawMonsterTile(i, j);
				if (tile.a == Treasure)
					DrawTreasureTile(i, j);
				if (tile.a == Spike)
					DrawTrapTile(i, j, false);
				if (tile.a == Death)
					DrawTrapTile(i, j, true);
				if (tile.a == Ankh) {
					glPushMatrix();
					glTranslatef(20, 0, -20);
					glScalef(40, 40, 40);
					GAME_STATE.textures.ankh_t.Bind();
					if (GAME_STATE.render.Cartoon)
						GAME_STATE.models.ankh->ShowC();
					else
						GAME_STATE.models.ankh->Show();
					glPopMatrix();
				}
				if (tile.a == Door) {
					glPushMatrix();
					glTranslatef(20, 0, -20);
					glPushMatrix();
					glScalef(40, 40, 40);
					if (tile.b != GateEntrance)
						glRotatef(180, 0, 1, 0);
					GAME_STATE.textures.sphinx_t.Bind();
					if (GAME_STATE.render.Cartoon)
						GAME_STATE.models.sphinx->ShowC();
					else
						GAME_STATE.models.sphinx->Show();
					glPopMatrix();
					glPopMatrix();

					if (tile.b == GateRiddle) {
						glPushMatrix();
						glTranslatef(20, 20, -20);
						glPushMatrix();
						glScalef(10, 10, 10);
						GAME_STATE.textures.scarab_t.Bind();
						glPushMatrix();
						glRotatef(qRot, 0, 1, 0);
						if (GAME_STATE.render.Cartoon)
							GAME_STATE.models.question->ShowC();
						else
							GAME_STATE.models.question->Show();
						qRot += 1.0;
						glPopMatrix();
						glPopMatrix();
						glPopMatrix();
					}

					if (tile.b == GateEntrance || tile.b == GateExit) {
						glPushMatrix();
						if (tile.b != GateEntrance)
							glTranslatef(39, 0, 0);

						float px = static_cast<float>((static_cast<int>(plasma * 100) % 100)) / 200.0f;

						GAME_STATE.textures.plasma_t.Bind();
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
						if (plasmaAni)
							plasma -= 0.022;
						glPopMatrix();
					}
				}
				if (tile.a == Ladder) {
					glPushMatrix();
					glTranslatef(20, 0, -25);
					glPushMatrix();
					glScalef(40, 40, 40);
					GAME_STATE.textures.column_t.Bind();
					if (GAME_STATE.render.Cartoon)
						GAME_STATE.models.column->ShowC();
					else
						GAME_STATE.models.column->Show();
					glPopMatrix();
					glPopMatrix();

					GAME_STATE.textures.plasma_t.Bind();
					glBegin(GL_QUADS);

					float px = static_cast<float>((static_cast<int>(plasma * 100) % 100)) / static_cast<float>(200.0);

					glNormal3f(0, 0, 1.0);
					glTexCoord2f(px, 0);
					glVertex3i(10, 0, -30);
					glTexCoord2f(px, 1);
					glVertex3i(10, 37, -30);
					glTexCoord2f(px + 1, 1);
					glVertex3i(30, 37, -30);
					glTexCoord2f(px + 1, 0);
					glVertex3i(30, 0, -30);
					glEnd();
					if (plasmaAni)
						plasma += 0.012;
				}
			}
			glTranslatef(40, 0, 0);
		}
		glTranslatef(RenderConfig::HUD_OFFSET_X, RenderConfig::TILE_SIZE, 0);
	}
	glPopMatrix();
}
