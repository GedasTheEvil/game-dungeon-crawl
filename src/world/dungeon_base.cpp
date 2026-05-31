#include "dungeon.h"
#include "../state/cashe.h"
#include "../input/gameplay_config.h"
#include <GL/gl.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>

extern Cashe c;

void normalize(VECTOR& v);

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
	m[index].m->X = static_cast<float>(m[index].orX);
	m[index].m->Y = static_cast<float>(m[index].orY);
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

	mL = false;

	for (int i = 0; i < CMaxMonsters; i++) {
		m[i].orX = -1;
		m[i].orY = -1;
		m[i].HP = 0;
		m[i].state = 1;
		m[i].facing_dir = 0;
	}

	char line[255];
	float shaderData[32][3];

	FILE* in = nullptr;
	in = fopen("Textures/ShaderD.bmp", "r");

	if (in) {
		for (int i = 0; i < 32; i++) {
			if (feof(in))
				break;

			if (fgets(line, 255, in) == nullptr)
				break;

			shaderData[i][0] = shaderData[i][1] = shaderData[i][2] = float(atof(line));
		}

		fclose(in);
	}

	glGenTextures(1, (GLuint*)&shaderTexture[0]);

	glBindTexture(GL_TEXTURE_1D, shaderTexture[0]);

	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGB, GL_FLOAT, shaderData);

	lightAngle.X = 0.0f;
	lightAngle.Y = 0.0f;
	lightAngle.Z = 1.0f;

	normalize(lightAngle);

	aniT = std::make_unique<timer>(50);

	for (int i = 0; i < CMaxMonsters; i++) {
		m[i].t.reset();
		m[i].at.reset();
	}
}
//======================================================================================
void Dungeon::UpdateMovementState() {
	if (Map(x, y).a != Ladder && !c.jumping) {
		if ((y - static_cast<float>(static_cast<int>(y))) > FALL_START_THRESHOLD || Map(x, y - 1).a != Wall) {
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
				c.jumping = false;
				c.falling = false;
			}
		}
	}
}
//======================================================================================
void Dungeon::Update() {
	UpdateMovementState();
	UpdateMonsters();
}
//======================================================================================
void Dungeon::Move(float dirX, float dirY, bool jump) {
	if (!c.falling && jump && Map(x, y).a != Ladder) {
		y = y + dirY;
		x = x + dirX;
		c.falling = true;
	}

	if (dirX > 0) {
		if (Map(x, y).a != Wall && Map(x + dirX + static_cast<float>(c.Player->scale / 60.0), y).a != Wall)
			x += dirX;
	} else if (Map(x, y).a != Wall && Map(x + dirX - static_cast<float>(c.Player->scale / 60.0), y).a != Wall)
		x += dirX;

	if (dirY > 0) {
		if (Map(x, y).a == Ladder && Map(x, y + dirY + static_cast<float>(c.Player->scale / 40.0)).a == Ladder)
			y += dirY;
	} else if (Map(x, y).a == Ladder && Map(x, y + dirY).a == Ladder)
		y += dirY;
}
//======================================================================================
int Dungeon::Type(float x, float y) { return Map(x, y).a; }
//======================================================================================
void Dungeon::getC(float& outX, float& outY) {
	outX = this->x;
	outY = this->y;
}
//======================================================================================
Dungeon::~Dungeon() { printf("Deleting Dungeon %p \n", (void*)this); }
