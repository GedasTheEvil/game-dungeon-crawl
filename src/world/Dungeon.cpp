#include "Dungeon.h"
#include "../state/cashe.h"
#include <GL/gl.h>
#include <fstream>
#include <cmath>
#include <memory>
#include "../input/gameplay_config.h"

float qRot = 0;

extern Cashe c;

// Math Functions
inline float DotProduct(VECTOR& V1, VECTOR& V2) { return V1.X * V2.X + V1.Y * V2.Y + V1.Z * V2.Z; }

inline float Magnitude(VECTOR& V) { return sqrtf(V.X * V.X + V.Y * V.Y + V.Z * V.Z); }

void Normalize(VECTOR& V);
void RotateVector(MATRIX& M, VECTOR& V, VECTOR& D);

monster* GetMbyType(int type); // pagalbinis metodas, nepriklauso klasei

float plasma = 0;

namespace {
bool ReadMapCells(std::istream& in, Tint* map, int cellCount) {
	for (int jj = 0; jj < cellCount; jj++) {
		in >> map[jj].a;
		in >> map[jj].b;
		in >> map[jj].c;
		if (!in)
			return false;
	}
	return true;
}
} // namespace

bool Dungeon::IsInBounds(int mapX, int mapY) const {
	return mapX >= 0 && mapX < kMapWidth && mapY >= 0 && mapY < kMapHeight;
}
//======================================================================================
int Dungeon::MapIndex(int mapX, int mapY) const {
	if (!IsInBounds(mapX, mapY))
		return 0;

	return kMapWidth * mapY + mapX;
}
//======================================================================================
Tint Dungeon::MapAt(int mapX, int mapY) const { return map[MapIndex(mapX, mapY)]; }
//======================================================================================
Tint Dungeon::Map(float x, float y) const { return MapAt((int)x, (int)y); }
//======================================================================================
void Dungeon::SetMapBAtPlayer(int value) { map[MapIndex((int)x, (int)y)].b = value; }
//======================================================================================
void Dungeon::SyncMonsterFromToken(int index) {
	m[index].m->DX = &x;
	m[index].m->DY = &y;
	m[index].m->setCords(m[index].x, m[index].y);
	m[index].m->X = m[index].orX;
	m[index].m->Y = m[index].orY;
	m[index].m->setModel(m[index].state);
	m[index].m->setFacingDir(m[index].facing_dir);
	m[index].m->HP = m[index].HP;
}
//======================================================================================
void Dungeon::SyncTokenFromMonster(int index, bool includePosition) {
	if (includePosition)
		m[index].m->GetCords(m[index].x, m[index].y);

	m[index].HP = m[index].m->HP;
	m[index].state = m[index].m->Model_state();
	m[index].facing_dir = m[index].m->FacingDir();
}
//======================================================================================
Dungeon::Dungeon() {
	x = 0;
	y = 0;
	c.falling = false;

	mL = 0;

	for (int i = 0; i < CMaxMonsters; i++) {
		m[i].orX = -1;
		m[i].orY = -1;
		m[i].HP = 0;
		m[i].state = 1;
		m[i].facing_dir = 0;
	}

	char Line[255];
	float shaderData[32][3];

	FILE* In = NULL;
	In = fopen("Textures/ShaderD.bmp", "r");

	if (In) {
		for (int i = 0; i < 32; i++) {
			if (feof(In))
				break;

			if (fgets(Line, 255, In) == NULL)
				break;

			shaderData[i][0] = shaderData[i][1] = shaderData[i][2] = float(atof(Line));
		}

		fclose(In);
	}

	glGenTextures(1, (GLuint*)&shaderTexture[0]);

	glBindTexture(GL_TEXTURE_1D, shaderTexture[0]);

	// For Crying Out Loud Don't Let OpenGL Use Bi/Trilinear Filtering!
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGB, GL_FLOAT, shaderData);

	lightAngle.X = 0.0f;
	lightAngle.Y = 0.0f;
	lightAngle.Z = 1.0f;

	Normalize(lightAngle);

	aniT = std::make_unique<timer>(50);

	for (int i = 0; i < CMaxMonsters; i++) {
		m[i].t.reset();
		m[i].at.reset();
	}
}
//======================================================================================
void Dungeon::DrawSegment(int seg, int l, int r, int u, int d) {

	c.blackTex.Bind();

	if (c.Cartoon) {

		float TmpShade;
		MATRIX TmpMatrix;
		VECTOR TmpVector, TmpNormal;

		glGetFloatv(GL_MODELVIEW_MATRIX, TmpMatrix.Data);

		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, shaderTexture[0]);

		switch (seg) {
		case 0:
			break;

		default:
			glBegin(/*GL_LINE_LOOP*/ GL_QUADS);
			TmpNormal.X = 0;
			TmpNormal.Y = 0;
			TmpNormal.Z = 1;

			RotateVector(TmpMatrix, TmpNormal, TmpVector);

			Normalize(TmpVector);

			TmpShade = DotProduct(TmpVector, lightAngle);

			if (TmpShade < 0.0f)
				TmpShade = 0.0f;
			glTexCoord1f(TmpShade);

			glVertex3i(0, 0, -40);

			TmpNormal.X = 0;
			TmpNormal.Y = 0.3;
			TmpNormal.Z = 1;

			RotateVector(TmpMatrix, TmpNormal, TmpVector);

			Normalize(TmpVector);

			TmpShade = DotProduct(TmpVector, lightAngle);

			if (TmpShade < 0.0f)
				TmpShade = 0.0f;
			glTexCoord1f(TmpShade);

			glVertex3i(0, 40, -40);

			TmpNormal.X = 0.2;
			TmpNormal.Y = 0.3;
			TmpNormal.Z = 1;

			RotateVector(TmpMatrix, TmpNormal, TmpVector);

			Normalize(TmpVector);

			TmpShade = DotProduct(TmpVector, lightAngle);

			if (TmpShade < 0.0f)
				TmpShade = 0.0f;
			glTexCoord1f(TmpShade);

			glVertex3i(40, 40, -40);

			TmpNormal.X = 0.2;
			TmpNormal.Y = 0.4;
			TmpNormal.Z = 0.5;

			RotateVector(TmpMatrix, TmpNormal, TmpVector);

			Normalize(TmpVector);

			TmpShade = DotProduct(TmpVector, lightAngle);

			if (TmpShade < 0.0f)
				TmpShade = 0.0f;
			glTexCoord1f(TmpShade);

			glVertex3i(40, 0, -40);
			glEnd();

			if (!l) {
				glBegin(/*GL_LINE_LOOP*/ GL_QUADS);

				TmpNormal.X = 1;
				TmpNormal.Y = 0;
				TmpNormal.Z = 0;

				RotateVector(TmpMatrix, TmpNormal, TmpVector);

				Normalize(TmpVector);

				TmpShade = DotProduct(TmpVector, lightAngle);

				if (TmpShade < 0.0f)
					TmpShade = 0.0f;
				glTexCoord1f(TmpShade);

				glVertex3i(0, 0, -40);
				glVertex3i(0, 40, -40);
				glVertex3i(0, 40, 0);
				glVertex3i(0, 0, 0);
				glEnd();
			}

			if (!r) {
				glBegin(/*GL_LINE_LOOP*/ GL_QUADS);
				TmpNormal.X = -1;
				TmpNormal.Y = 0;
				TmpNormal.Z = 0;

				RotateVector(TmpMatrix, TmpNormal, TmpVector);

				Normalize(TmpVector);

				TmpShade = DotProduct(TmpVector, lightAngle);

				if (TmpShade < 0.0f)
					TmpShade = 0.0f;
				glTexCoord1f(TmpShade);

				glVertex3i(40, 0, -40);
				glVertex3i(40, 40, -40);
				glVertex3i(40, 40, 0);
				glVertex3i(40, 0, 0);
				glEnd();
			}

			if (!u) {
				glBegin(/*GL_LINE_LOOP*/ GL_QUADS);

				TmpNormal.X = 0;
				TmpNormal.Y = -1;
				TmpNormal.Z = 0;

				RotateVector(TmpMatrix, TmpNormal, TmpVector);

				Normalize(TmpVector);

				TmpShade = DotProduct(TmpVector, lightAngle);

				if (TmpShade < 0.0f)
					TmpShade = 0.0f;
				glTexCoord1f(TmpShade);

				glVertex3i(0, 40, -40);
				glVertex3i(40, 40, -40);
				glVertex3i(40, 40, 0);
				glVertex3i(0, 40, 0);
				glEnd();
			}

			if (!d) {
				glBegin(/*GL_LINE_LOOP*/ GL_QUADS);

				TmpNormal.X = 0;
				TmpNormal.Y = 1;
				TmpNormal.Z = 0;

				RotateVector(TmpMatrix, TmpNormal, TmpVector);

				Normalize(TmpVector);

				TmpShade = DotProduct(TmpVector, lightAngle);

				if (TmpShade < 0.0f)
					TmpShade = 0.0f;
				glTexCoord1f(TmpShade);

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
		switch (seg) {
		case 0:
			glDisable(GL_BLEND);
			c.black_t.Bind();
			glBegin(/*GL_LINE_LOOP*/ GL_QUADS);
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
			glBegin(/*GL_LINE_LOOP*/ GL_QUADS);
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

			if (!l) {
				glBegin(/*GL_LINE_LOOP*/ GL_QUADS);
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

			if (!r) {
				glBegin(/*GL_LINE_LOOP*/ GL_QUADS);
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

			if (!u) {
				glBegin(/*GL_LINE_LOOP*/ GL_QUADS);
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

			if (!d) {
				glBegin(/*GL_LINE_LOOP*/ GL_QUADS);
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
bool Dungeon::Load(const char* filename) {
	std::ifstream f(filename);
	if (!f)
		return 0;

	int header;
	f >> header;
	if (header != kMapCellCount) {
		printf("Wrong map header. Expected '%d', got %d\n", kMapCellCount, header);
		return 0;
	}

	if (!ReadMapCells(f, map, kMapCellCount))
		return 0;
	f.close();

	// find start point
	for (int j = 0; j < kMapHeight; j++)
		for (int i = 0; i < kMapWidth; i++)
			if (map[MapIndex(i, j)].a == Door && map[MapIndex(i, j)].b == GateEntrance) {
				x = i;
				y = j;
			}

	return 1;
}
//======================================================================================
bool Dungeon::LoadDump(std::ifstream& f) {
	f >> x >> y;
	if (!f)
		return 0;

	int header;
	f >> header;
	if (header != kMapCellCount) {
		printf("Wrong header. Expected '%d', got %d\n", kMapCellCount, header);
		return 0;
	}

	if (!ReadMapCells(f, map, kMapCellCount))
		return 0;

	return 1;
}
//======================================================================================
void Dungeon::UpdateMovementState() {
	if (Map(x, y).a != Ladder && !c.jumping) {
		if ((y - (int)y) > FALL_START_THRESHOLD || Map(x, y - 1).a != Wall) {
			if (c.fall_inc->TimePassed())
				y -= FALL_STEP;
			c.falling = true;
		} else {
			c.falling = false;
		}
	}

	if (c.jumping) {
		if (c.jump_inc->TimePassed()) {
			if (c.jump_dir_x != 0)
				Move(c.jump_dir_x * c.jump_speed, 0);

			y += c.jump_vel;
			c.jump_vel -= JUMP_GRAVITY_STEP;

			if (c.jump_vel <= 0 && y <= c.jump_start_y) {
				y = c.jump_start_y;
				c.jumping = 0;
				c.falling = false;
			}
		}
	}
}
//======================================================================================
void Dungeon::UpdateMonsters() {
	for (int a = 0; a < CMaxMonsters; a++) {
		if (m[a].orX == -1 || m[a].orY == -1)
			continue;

		SyncMonsterFromToken(a);

		if (m[a].m->Alive() && !c.IHaveWon && m[a].t->TimePassed()) {
			if (!m[a].m->Seek())
				if (m[a].at->TimePassed())
					m[a].m->Attack();
			SyncTokenFromMonster(a, true);
		}
	}
}
//======================================================================================
void Dungeon::Update() {
	UpdateMovementState();
	UpdateMonsters();
}
//======================================================================================
void Dungeon::DrawMonsterTile(int i, int j) {
	SpawnMonster(i, j);
	for (int a = 0; a < CMaxMonsters; a++) {
		if (m[a].orX == i && m[a].orY == j) {
			SyncMonsterFromToken(a);
			glPushMatrix();
			glTranslatef(40, 0, 10);
			m[a].m->Draw();
			glPopMatrix();
			SyncTokenFromMonster(a, false);
		}
	}
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
	tileTrap->setCords(i, j);
	tileTrap->Show();

	glPopMatrix();
}
//======================================================================================
void Dungeon::Draw() {

	bool plasma_ani;
	if (aniT->TimePassed())
		plasma_ani = 1;
	else
		plasma_ani = 0;

	/// @name Drawing

	glPushMatrix();
	glTranslatef(-40 * (x - (int)x), -40 * (y - (int)y), 0);
	glTranslatef(80, -120, 0); // 80,-120

	for (int j = (int)y - 3; j < (int)y + 3; j++) {
		for (int i = (int)x - 3; i < (int)x + 5; i++) {
			if (IsInBounds(i, j)) {
				const Tint tile = MapAt(i, j);
				DrawSegment(tile.a, MapAt(i - 1, j).a, MapAt(i + 1, j).a, MapAt(i, j + 1).a, MapAt(i, j - 1).a); // map

				if (tile.a == Monster) // mob
				{
					DrawMonsterTile(i, j);
				} // end of monster
				if (tile.a == Treasure) {
					DrawTreasureTile(i, j);
				} // end of treasure
				if (tile.a == Spike) {
					DrawTrapTile(i, j, false);
				} // end of spike trap
				if (tile.a == Death) {
					DrawTrapTile(i, j, true);
				} // end of death trap
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
						// 				  glEnable(GL_BLEND);
						glPushMatrix();
						glRotatef(qRot, 0, 1, 0);
						if (c.Cartoon)
							c.question->ShowC();
						else
							c.question->Show();
						qRot += 1.0;
						glPopMatrix();
						// 				  glDisable(GL_BLEND);
						glPopMatrix();
						glPopMatrix();
					}

					if (tile.b == GateEntrance || tile.b == GateExit) {
						glPushMatrix();
						if (tile.b != GateEntrance)
							glTranslatef(39, 0, 0);

						// testing plasma
						float px = ((int)(plasma * 100) % 100) / 200.0;

						c.plasma_t.Bind();
						glBegin(/*GL_LINE_LOOP*/ GL_QUADS);
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
						glEnd();
						if (plasma_ani)
							plasma -= 0.022;
						glPopMatrix();
					}

				} // end of gate
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

					// testing plasma
					// 			    glEnable(GL_BLEND);
					c.plasma_t.Bind();
					glBegin(GL_QUADS);

					float px = ((int)(plasma * 100) % 100) / 200.0;

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
					// 			    glDisable(GL_BLEND);
					if (plasma_ani)
						plasma += 0.012;
				}
			}
			glTranslatef(40, 0, 0);
		}
		glTranslatef(-320, 40, 0); // -240,40
	}
	glPopMatrix();
}
//======================================================================================
void Dungeon::Move(float dirX, float dirY, bool jump) {
	if (!c.falling && jump && Map(x, y).a != Ladder) // needs more work
	{
		y = /*(int)*/ y + dirY;
		x = /*(int)*/ x + dirX;
		c.falling = true;
	}

	if (dirX > 0) {
		if (Map(x, y).a != Wall && Map(x + dirX + c.Player->scale / 60.0, y).a != Wall)
			x += dirX;
	} else if (Map(x, y).a != Wall && Map(x + dirX - c.Player->scale / 60.0, y).a != Wall)
		x += dirX;

	if (dirY > 0) {
		if (Map(x, y).a == Ladder && Map(x, y + dirY + c.Player->scale / 40.0).a == Ladder)
			y += dirY;
	} else if (Map(x, y).a == Ladder && Map(x, y + dirY).a == Ladder)
		y += dirY;
}
//======================================================================================
int Dungeon::Type(float x, float y) { return Map(x, y).a; }
//======================================================================================
void Dungeon::getC(float& x, float& y) {
	x = this->x;
	y = this->y;
}
//======================================================================================
void Dungeon::GetAttack(int dmg, int range) {
	for (int i = 0; i < CMaxMonsters; i++) {
		if (m[i].orX != -1 && m[i].orY != -1) {
			SyncMonsterFromToken(i);
			if (m[i].m->Alive() && m[i].m->Nearby(x, y, range)) {
				m[i].m->getHit(dmg);
				SyncTokenFromMonster(i, true);
				break; // hit only one monster at once;
			}
		}
	}
}
//======================================================================================
void Dungeon::GetPickUp() {
	if (Map(x, y).a == Treasure) {
		c.invent->GetItem(Map(x, y).b, Map(x, y).c);
		map[MapIndex((int)x, (int)y)].a = Empty;
		if (Map(x, y).b) {
			sprintf(c.status, "Picked up an item  \n");
			c.status_timer->Reset();
		}
	}
}
//======================================================================================
void Dungeon::GetRiddle() {
	if (Map(x, y).a == Ankh) {
		c.IHaveWon = 1;
		return;
	}

	if (Map(x, y).a == Door && Map(x, y).b == GateRiddle) {
		c.rid->GetRiddle();
		c.rid->show = 1;
		SetMapBAtPlayer(GateEmpty);
	} else if (Map(x, y).a == Door && Map(x, y).b == GateExit) {
		c.curMap++;

		char mapName[40];

		sprintf(mapName, "Levels/lvl%d", c.curMap);

		Load(mapName);
	}
}
//======================================================================================
monster* GetMbyType(int type) {
	if (type == 1)
		return c.scarab.get();
	if (type == 2)
		return c.worm.get();
	if (type == 3)
		return c.plant.get();
	if (type == 4)
		return c.anubis.get();

	return c.Player.get(); // debug
}
//======================================================================================
void Dungeon::InitializeMonsterSlot(int index, int i, int j) {
	m[index].m = GetMbyType(Map(i, j).b);
	m[index].m->DX = &x;
	m[index].m->DY = &y;
	m[index].m->X = i;
	m[index].m->Y = j;
	m[index].orX = i;
	m[index].orY = j;
	m[index].HP = m[index].m->MaxHP;
	m[index].m->GetCords(m[index].x, m[index].y);
	m[index].state = 1;
	m[index].facing_dir = 0;
	m[index].frame = 1;
	if (!m[index].t)
		m[index].t = std::make_unique<timer>(70);
	if (!m[index].at)
		m[index].at = std::make_unique<timer>(800);
}
//======================================================================================
bool Dungeon::SpawnMonster(int i, int j) {
	int index = -1;

	for (int a = 0; a < CMaxMonsters; a++) // find if the monster instance is already spawned
		if (m[a].orX == i && m[a].orY == j) {
			index = a;
			break;
		}

	if (index != -1)
		return 0; // no need spawning, we got it :)

	for (int a = 0; a < CMaxMonsters; a++)
		if (m[a].orX == -1 && m[a].orY == -1) {
			index = a;
			break;
		}

	if (index != -1) // gavom laisva slot'a, spawninam
	{
		InitializeMonsterSlot(index, i, j);
		return 1;
	}
	/// pajimam slot'a is negyvu mobu (untested)

	for (int a = 0; a < CMaxMonsters; a++)
		if (m[a].HP < 1) {
			index = a;
			break;
		}

	if (index != -1) // gavom laisva slot'a, spawninam
	{
		if (m[index].orX != i || m[index].orY != j) // avoid spawning a monster right after it's death
		{
			InitializeMonsterSlot(index, i, j);
			return 1;
		}
	}

	return 0;
}

//======================================================================================
Dungeon::~Dungeon() { printf("Deleting Dungeon %p \n", (void*)this); }
//======================================================================================
void Dungeon::Dump(std::ofstream& f) {
	f << x << " " << y << " ";

	f << kMapCellCount << " ";

	for (int l = 0; l < kMapCellCount; l++)
		f << map[l].a << " " << map[l].b << " " << map[l].c << " ";
}
//======================================================================================
