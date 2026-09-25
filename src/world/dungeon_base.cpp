#include "dungeon.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../input/gameplay_config.h"
#include "../core/logger.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>

void normalize(VECTOR& v);

namespace {
// Map x within a cell at which the drawn player (always at the screen centre) is in front of the ladder:
// tile i is drawn from 40 * (i - mapX) - 2 and the ladder stands at its middle (Dungeon::Draw).
constexpr float LADDER_GRIP_X = 0.45f;
constexpr float LADDER_REACH = 0.2f; // closer to LADDER_GRIP_X than this (tiles), the player holds on to the ladder
constexpr float STANDING_EPSILON = 0.05f; // above the floor by less than this still counts as standing on it
constexpr float CLIMB_SIDE_RATE = 0.5f;
} // namespace

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
	m[index].m->restoreAnimations(m[index].state, m[index].anim);
	m[index].m->setFacingDir(m[index].facing_dir);
	m[index].m->health = m[index].HP;
	m[index].m->useBlood(m[index].blood.get());
}
//======================================================================================
void Dungeon::SyncTokenFromMonster(int index, bool includePosition) {
	if (includePosition)
		m[index].m->GetCords(m[index].mapX, m[index].mapY);

	m[index].HP = m[index].m->health;
	m[index].state = m[index].m->Model_state();
	m[index].anim = m[index].m->animations();
	m[index].facing_dir = m[index].m->FacingDir();
	m[index].m->useBlood(nullptr);
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
	in = fopen("Textures/ShaderD.txt", "r");

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
			!isSolidTile(Map(mapX, mapY - 1))) {
			JumpState& jump = GAME_STATE.Player->jump;
			if (jump.fall_inc->TimePassed()) {
				float floorY = std::floor(mapY);
				mapY -= jump.fall_velocity;
				if (mapY < floorY && isSolidTile(Map(mapX, floorY - 1)))
					mapY = floorY; // landed: don't sink into the floor tile
				jump.fall_velocity = std::min(jump.fall_velocity + FALL_GRAVITY_STEP, FALL_MAX_STEP);
			}
			jump.falling = true;
		} else {
			mapY = std::floor(mapY); // drop the sub-threshold remainder left by the last step
			GAME_STATE.Player->jump.falling = false;
			GAME_STATE.Player->jump.fall_velocity = FALL_STEP;
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
	updateMechanisms();
	UpdateMonsters();
}
//======================================================================================
void Dungeon::Move(float dirX, float dirY, bool jump) {
	if (!GAME_STATE.Player->jump.falling && jump && Map(mapX, mapY).a != Ladder) {
		mapY = mapY + dirY;
		mapX = mapX + dirX;
		GAME_STATE.Player->jump.falling = true;
	}

	if (dirX != 0) {
		float halfWidth = static_cast<float>(GAME_STATE.Player->scale / 60.0);
		float probeX = mapX + dirX + (dirX > 0 ? halfWidth : -halfWidth);
		if (!isSolidTile(Map(mapX, mapY)) && !isSolidTile(Map(probeX, mapY)))
			mapX += dirX;
		else if (Map(probeX, mapY).a == Gate)
			bumpGate(static_cast<int>(probeX), static_cast<int>(mapY));
	}

	if (dirY > 0) {
		if (Map(mapX, mapY).a == Ladder &&
			Map(mapX, mapY + dirY + static_cast<float>(GAME_STATE.Player->scale / 40.0)).a == Ladder)
			mapY += dirY;
	} else if (Map(mapX, mapY).a == Ladder && Map(mapX, mapY + dirY).a == Ladder)
		mapY += dirY;

	// Climbing pulls the player over to the ladder, so the hands reach the rungs.
	if (dirY != 0 && Map(mapX, mapY).a == Ladder) {
		float offset = std::floor(mapX) + LADDER_GRIP_X - mapX;
		mapX += std::clamp(offset, -std::fabs(dirY), std::fabs(dirY));
	}
}
//======================================================================================
bool Dungeon::PlayerOnLadder() const {
	if (Map(mapX, mapY).a != Ladder || std::fabs(mapX - std::floor(mapX) - LADDER_GRIP_X) > LADDER_REACH)
		return false;
	bool floorBelow = isSolidTile(Map(mapX, mapY - 1));
	return !floorBelow || mapY - std::floor(mapY) >= STANDING_EPSILON;
}
//======================================================================================
float Dungeon::ClimbPhase() const { return mapY + CLIMB_SIDE_RATE * mapX; }
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
