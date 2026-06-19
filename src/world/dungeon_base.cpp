#include "dungeon.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../input/gameplay_config.h"
#include "../core/logger.h"
#include <GL/gl.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>

void normalize(VECTOR& v);

bool Dungeon::IsInBounds(int col, int row) const { return col >= 0 && col < kMapWidth && row >= 0 && row < kMapHeight; }
//======================================================================================
int Dungeon::MapIndex(int col, int row) const {
	if (!IsInBounds(col, row))
		return 0;

	return kMapWidth * row + col;
}
//======================================================================================
Tint Dungeon::MapAt(int col, int row) const { return map[MapIndex(col, row)]; }
//======================================================================================
Tint Dungeon::Map(float x, float y) const { return MapAt(static_cast<int>(x), static_cast<int>(y)); }
//======================================================================================
void Dungeon::SetMapBAtPlayer(int value) { map[MapIndex(static_cast<int>(mapX), static_cast<int>(mapY))].b = value; }
//======================================================================================
void Dungeon::SyncMonsterFromToken(int index) {
	m[index].m->dungeonCamX = &mapX;
	m[index].m->dungeonCamY = &mapY;
	m[index].m->setCords(m[index].mapX, m[index].mapY);
	m[index].m->tileOriginX = static_cast<float>(m[index].orX);
	m[index].m->tileOriginY = static_cast<float>(m[index].orY);
	m[index].m->setModel(m[index].state);
	m[index].m->setFacingDir(m[index].facing_dir);
	m[index].m->health = m[index].HP;
}
//======================================================================================
void Dungeon::SyncTokenFromMonster(int index, bool includePosition) {
	if (includePosition)
		m[index].m->GetCords(m[index].mapX, m[index].mapY);

	m[index].HP = m[index].m->health;
	m[index].state = m[index].m->Model_state();
	m[index].facing_dir = m[index].m->FacingDir();
}
//======================================================================================
Dungeon::Dungeon() {
	mapX = 0;
	mapY = 0;
	// Don't access GAME_STATE during construction to avoid circular dependency
	// GAME_STATE.falling will be set during proper initialization

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

	glGenTextures(1, reinterpret_cast<GLuint*>(&shaderTexture[0]));

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
	if (Map(mapX, mapY).a != Ladder && !GAME_STATE.Player->jump.jumping) {
		if ((mapY - static_cast<float>(static_cast<int>(mapY))) > FALL_START_THRESHOLD ||
			Map(mapX, mapY - 1).a != Wall) {
			if (GAME_STATE.Player->jump.fall_inc->TimePassed())
				mapY -= FALL_STEP;
			GAME_STATE.Player->jump.falling = true;
		} else {
			GAME_STATE.Player->jump.falling = false;
		}
	}

	if (GAME_STATE.Player->jump.jumping) {
		if (GAME_STATE.Player->jump.jump_inc->TimePassed()) {
			if (GAME_STATE.Player->jump.dir_x != 0)
				Move(GAME_STATE.Player->jump.dir_x * GAME_STATE.Player->jump.speed, 0);

			mapY += GAME_STATE.Player->jump.velocity;
			GAME_STATE.Player->jump.velocity -= JUMP_GRAVITY_STEP;

			if (GAME_STATE.Player->jump.velocity <= 0 && mapY <= GAME_STATE.Player->jump.start_y) {
				mapY = GAME_STATE.Player->jump.start_y;
				GAME_STATE.Player->jump.jumping = false;
				GAME_STATE.Player->jump.falling = false;
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
	if (!GAME_STATE.Player->jump.falling && jump && Map(mapX, mapY).a != Ladder) {
		mapY = mapY + dirY;
		mapX = mapX + dirX;
		GAME_STATE.Player->jump.falling = true;
	}

	if (dirX > 0) {
		if (Map(mapX, mapY).a != Wall &&
			Map(mapX + dirX + static_cast<float>(GAME_STATE.Player->scale / 60.0), mapY).a != Wall)
			mapX += dirX;
	} else if (Map(mapX, mapY).a != Wall &&
			   Map(mapX + dirX - static_cast<float>(GAME_STATE.Player->scale / 60.0), mapY).a != Wall)
		mapX += dirX;

	if (dirY > 0) {
		if (Map(mapX, mapY).a == Ladder &&
			Map(mapX, mapY + dirY + static_cast<float>(GAME_STATE.Player->scale / 40.0)).a == Ladder)
			mapY += dirY;
	} else if (Map(mapX, mapY).a == Ladder && Map(mapX, mapY + dirY).a == Ladder)
		mapY += dirY;
}
//======================================================================================
int Dungeon::Type(float x, float y) { return Map(x, y).a; }
//======================================================================================
void Dungeon::getC(float& outX, float& outY) {
	outX = mapX;
	outY = mapY;
}
//======================================================================================
Dungeon::~Dungeon() {
	void* selfPtr = this;
	LOG_DEBUGF("world", "Deleting Dungeon %p", selfPtr);
}
