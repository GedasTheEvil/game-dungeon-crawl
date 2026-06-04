#include "dungeon.h"
#include "../state/cashe.h"
#include <GL/gl.h>
#include <cmath>

extern Cashe c;

float qRot = 0;
float plasma = 0;

inline float dotProduct(VECTOR& v1, VECTOR& v2) { return v1.X * v2.X + v1.Y * v2.Y + v1.Z * v2.Z; }
void normalize(VECTOR& v);
void rotateVector(MATRIX& m, VECTOR& v, VECTOR& d);

void Dungeon::DrawSegment(int type, int leftWallType, int rightWallType, int upWallType, int downWallType) {
	c.blackTex.Bind();

	if (c.Cartoon) {
		float tmpShade;
		MATRIX tmpMatrix;
		VECTOR tmpVector, tmpNormal;

		glGetFloatv(GL_MODELVIEW_MATRIX, tmpMatrix.Data);

		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, shaderTexture[0]);

		switch (type) {
		case 0:
			break;

		default:
			glBegin(GL_QUADS);
			tmpNormal.X = 0;
			tmpNormal.Y = 0;
			tmpNormal.Z = 1;

			rotateVector(tmpMatrix, tmpNormal, tmpVector);
			normalize(tmpVector);
			tmpShade = dotProduct(tmpVector, lightAngle);
			if (tmpShade < 0.0f)
				tmpShade = 0.0f;
			glTexCoord1f(tmpShade);
			glVertex3i(0, 0, -40);

			tmpNormal.X = 0;
			tmpNormal.Y = 0.3;
			tmpNormal.Z = 1;
			rotateVector(tmpMatrix, tmpNormal, tmpVector);
			normalize(tmpVector);
			tmpShade = dotProduct(tmpVector, lightAngle);
			if (tmpShade < 0.0f)
				tmpShade = 0.0f;
			glTexCoord1f(tmpShade);
			glVertex3i(0, 40, -40);

			tmpNormal.X = 0.2;
			tmpNormal.Y = 0.3;
			tmpNormal.Z = 1;
			rotateVector(tmpMatrix, tmpNormal, tmpVector);
			normalize(tmpVector);
			tmpShade = dotProduct(tmpVector, lightAngle);
			if (tmpShade < 0.0f)
				tmpShade = 0.0f;
			glTexCoord1f(tmpShade);
			glVertex3i(40, 40, -40);

			tmpNormal.X = 0.2;
			tmpNormal.Y = 0.4;
			tmpNormal.Z = 0.5;
			rotateVector(tmpMatrix, tmpNormal, tmpVector);
			normalize(tmpVector);
			tmpShade = dotProduct(tmpVector, lightAngle);
			if (tmpShade < 0.0f)
				tmpShade = 0.0f;
			glTexCoord1f(tmpShade);
			glVertex3i(40, 0, -40);
			glEnd();

			if (!leftWallType) {
				glBegin(GL_QUADS);
				tmpNormal.X = 1;
				tmpNormal.Y = 0;
				tmpNormal.Z = 0;
				rotateVector(tmpMatrix, tmpNormal, tmpVector);
				normalize(tmpVector);
				tmpShade = dotProduct(tmpVector, lightAngle);
				if (tmpShade < 0.0f)
					tmpShade = 0.0f;
				glTexCoord1f(tmpShade);
				glVertex3i(0, 0, -40);
				glVertex3i(0, 40, -40);
				glVertex3i(0, 40, 0);
				glVertex3i(0, 0, 0);
				glEnd();
			}

			if (!rightWallType) {
				glBegin(GL_QUADS);
				tmpNormal.X = -1;
				tmpNormal.Y = 0;
				tmpNormal.Z = 0;
				rotateVector(tmpMatrix, tmpNormal, tmpVector);
				normalize(tmpVector);
				tmpShade = dotProduct(tmpVector, lightAngle);
				if (tmpShade < 0.0f)
					tmpShade = 0.0f;
				glTexCoord1f(tmpShade);
				glVertex3i(40, 0, -40);
				glVertex3i(40, 40, -40);
				glVertex3i(40, 40, 0);
				glVertex3i(40, 0, 0);
				glEnd();
			}

			if (!upWallType) {
				glBegin(GL_QUADS);
				tmpNormal.X = 0;
				tmpNormal.Y = -1;
				tmpNormal.Z = 0;
				rotateVector(tmpMatrix, tmpNormal, tmpVector);
				normalize(tmpVector);
				tmpShade = dotProduct(tmpVector, lightAngle);
				if (tmpShade < 0.0f)
					tmpShade = 0.0f;
				glTexCoord1f(tmpShade);
				glVertex3i(0, 40, -40);
				glVertex3i(40, 40, -40);
				glVertex3i(40, 40, 0);
				glVertex3i(0, 40, 0);
				glEnd();
			}

			if (!downWallType) {
				glBegin(GL_QUADS);
				tmpNormal.X = 0;
				tmpNormal.Y = 1;
				tmpNormal.Z = 0;
				rotateVector(tmpMatrix, tmpNormal, tmpVector);
				normalize(tmpVector);
				tmpShade = dotProduct(tmpVector, lightAngle);
				if (tmpShade < 0.0f)
					tmpShade = 0.0f;
				glTexCoord1f(tmpShade);
				glVertex3i(0, 0, -40);
				glVertex3i(40, 0, -40);
				glVertex3i(40, 0, 0);
				glVertex3i(0, 0, 0);
				glEnd();
			}

			break;
		}

		glDisable(GL_TEXTURE_1D);
	}

	if (!c.Cartoon || c.Orig_model) {
		if (c.Orig_model) {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			glColor4f(1, 1, 1, 0.4);
		}
		switch (type) {
		case 0:
			glDisable(GL_BLEND);
			c.black_t.Bind();
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
			break;

		default:
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

			if (!leftWallType) {
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

			if (!rightWallType) {
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

			if (!upWallType) {
				glBegin(GL_QUADS);
				glNormal3f(0, -1, 0);
				glTexCoord2f(0.3, 0.3);
				glVertex3i(0, 40, -40);
				glTexCoord2f(0.3, 0.7);
				glVertex3i(40, 40, -40);
				glTexCoord2f(0.7, 0.7);
				glVertex3i(40, 40, 0);
				glTexCoord2f(0.7, 0.3);
				glVertex3i(0, 40, 0);
				glEnd();
			}

			if (!downWallType) {
				glBegin(GL_QUADS);
				glNormal3f(0, 1, 0);
				glTexCoord2f(0.3, 0.3);
				glVertex3i(0, 0, -40);
				glTexCoord2f(0.3, 0.7);
				glVertex3i(40, 0, -40);
				glTexCoord2f(0.7, 0.7);
				glVertex3i(40, 0, 0);
				glTexCoord2f(0.7, 0.3);
				glVertex3i(0, 0, 0);
				glEnd();
			}

			break;
		}
		if (c.Orig_model)
			glDisable(GL_BLEND);
	}

	glColor3f(1, 1, 1);
}
//======================================================================================
void Dungeon::DrawTreasureTile(int i, int j) {
	const Tint tile = MapAt(i, j);

	glPushMatrix();
	glTranslatef(20, 0, 10);
	c.chest->Draw();

	if (tile.b == 3) {
		c.potion->Draw();
		c.potion->rotA++;
	}

	if (tile.b == 2) {
		c.bow->Draw();
		c.bow->rotA++;
	}

	if (tile.b == 1) {
		if (tile.c == 0) {
			c.club->scale = 10;
			c.club->Draw();
			c.club->rotA++;
		}
		if (tile.c == 1) {
			c.sword->Draw();
			c.sword->rotA++;
		}
		if (tile.c == 2) {
			c.spear->Draw();
			c.spear->rotA++;
		}
	}

	glPopMatrix();
}
//======================================================================================
void Dungeon::DrawTrapTile(int i, int j, bool isDeathTrap) {
	glPushMatrix();
	glTranslatef(20, 0, 10);

	trap* tileTrap = isDeathTrap ? c.DeathTrap.get() : c.TrapD.get();
	tileTrap->DX = &x;
	tileTrap->DY = &y;
	tileTrap->setCords(static_cast<float>(i), static_cast<float>(j));
	tileTrap->Show();

	glPopMatrix();
}
//======================================================================================
void Dungeon::Draw() {
	bool plasmaAni;
	plasmaAni = aniT->TimePassed();

	glPushMatrix();
	glTranslatef(static_cast<float>(-40) * (x - static_cast<float>(static_cast<int>(x))),
				 static_cast<float>(-40) * (y - static_cast<float>(static_cast<int>(y))), static_cast<float>(0));
	glTranslatef(80, -120, 0);

	for (int j = (int)y - 3; j < (int)y + 3; j++) {
		for (int i = (int)x - 3; i < (int)x + 5; i++) {
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
					c.ankh_t.Bind();
					if (c.Cartoon)
						c.ankh->ShowC();
					else
						c.ankh->Show();
					glPopMatrix();
				}
				if (tile.a == Door) {
					glPushMatrix();
					glTranslatef(20, 0, -20);
					glPushMatrix();
					glScalef(40, 40, 40);
					if (tile.b != GateEntrance)
						glRotatef(180, 0, 1, 0);
					c.sphinx_t.Bind();
					if (c.Cartoon)
						c.sphinx->ShowC();
					else
						c.sphinx->Show();
					glPopMatrix();
					glPopMatrix();

					if (tile.b == GateRiddle) {
						glPushMatrix();
						glTranslatef(20, 20, -20);
						glPushMatrix();
						glScalef(10, 10, 10);
						c.scarab_t.Bind();
						glPushMatrix();
						glRotatef(qRot, 0, 1, 0);
						if (c.Cartoon)
							c.question->ShowC();
						else
							c.question->Show();
						qRot += 1.0;
						glPopMatrix();
						glPopMatrix();
						glPopMatrix();
					}

					if (tile.b == GateEntrance || tile.b == GateExit) {
						glPushMatrix();
						if (tile.b != GateEntrance)
							glTranslatef(39, 0, 0);

						float px = static_cast<float>(((int)(plasma * 100) % 100)) / 200.0f;

						c.plasma_t.Bind();
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
					c.column_t.Bind();
					if (c.Cartoon)
						c.column->ShowC();
					else
						c.column->Show();
					glPopMatrix();
					glPopMatrix();

					c.plasma_t.Bind();
					glBegin(GL_QUADS);

					float px = static_cast<float>(((int)(plasma * 100) % 100)) / static_cast<float>(200.0);

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
		glTranslatef(-320, 40, 0);
	}
	glPopMatrix();
}
