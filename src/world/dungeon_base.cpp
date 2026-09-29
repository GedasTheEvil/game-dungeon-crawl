#include "dungeon.h"
#include "../state/game_state.h"
#include "../core/gameplay_config.h"
#include "../core/logger.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>

namespace {
// Map x within a cell at which the drawn player (always at the screen centre) is in front of the ladder:
// tile i is drawn from 40 * (i - mapX) - 2 and the ladder stands at its middle (Dungeon::Draw).
constexpr float LADDER_GRIP_X = 0.45f;
constexpr float LADDER_REACH = 0.2f; // closer to LADDER_GRIP_X than this (tiles), the player holds on to the ladder
constexpr float STANDING_EPSILON = 0.05f; // above the floor by less than this still counts as standing on it
constexpr float CLIMB_SIDE_RATE = 0.5f;
} // namespace

bool Dungeon::IsInBounds(int col, int row) const { return col >= 0 && col < MAP_WIDTH && row >= 0 && row < MAP_HEIGHT; }
//======================================================================================
int Dungeon::MapIndex(int col, int row) const {
	if (!IsInBounds(col, row))
		return 0;

	return MAP_WIDTH * row + col;
}
//======================================================================================
Tile Dungeon::MapAt(int col, int row) const { return map[MapIndex(col, row)]; }
//======================================================================================
Tile Dungeon::Map(float x, float y) const { return MapAt(static_cast<int>(x), static_cast<int>(y)); }
//======================================================================================
void Dungeon::SetMapBAtPlayer(int value) { map[MapIndex(static_cast<int>(mapX), static_cast<int>(mapY))].attr = value; }
//======================================================================================
void Dungeon::resetPlayerMotion() {
	JumpState& jump = Game().player->jump;
	jump.jumping = false;
	jump.falling = false;
	jump.velocity = 0.f;
	jump.fall_velocity = FALL_STEP;
}
//======================================================================================
void Dungeon::exploreAroundPlayer() {
	int col = static_cast<int>(mapX);
	int row = static_cast<int>(mapY);
	for (int j = row - EXPLORE_RADIUS; j <= row + EXPLORE_RADIUS; j++)
		for (int i = col - EXPLORE_RADIUS; i <= col + EXPLORE_RADIUS; i++)
			if (IsInBounds(i, j))
				explored[MapIndex(i, j)] = true;
}
//======================================================================================
Dungeon::Dungeon() {
	mapX = 0;
	mapY = 0;
}
//======================================================================================
void Dungeon::UpdateMovementState() {
	if (Map(mapX, mapY).type != Ladder && !Game().player->jump.jumping) {
		if ((mapY - static_cast<float>(static_cast<int>(mapY))) > FALL_START_THRESHOLD ||
			!isSolidTile(Map(mapX, mapY - 1))) {
			JumpState& jump = Game().player->jump;
			if (jump.fall_inc.TimePassed()) {
				float floorY = std::floor(mapY);
				mapY -= jump.fall_velocity;
				if (mapY < floorY && isSolidTile(Map(mapX, floorY - 1)))
					mapY = floorY; // landed: don't sink into the floor tile
				jump.fall_velocity = std::min(jump.fall_velocity + FALL_GRAVITY_STEP, FALL_MAX_STEP);
			}
			jump.falling = true;
		} else {
			mapY = std::floor(mapY); // drop the sub-threshold remainder left by the last step
			Game().player->jump.falling = false;
			Game().player->jump.fall_velocity = FALL_STEP;
		}
	}

	if (Game().player->jump.jumping) {
		if (Game().player->jump.jump_inc.TimePassed()) {
			if (Game().player->jump.dir_x != 0)
				Move(Game().player->jump.dir_x * Game().player->jump.speed, 0);

			mapY += Game().player->jump.velocity;
			Game().player->jump.velocity -= JUMP_GRAVITY_STEP;

			if (Game().player->jump.velocity <= 0 && mapY <= Game().player->jump.start_y) {
				mapY = Game().player->jump.start_y;
				Game().player->jump.jumping = false;
				Game().player->jump.falling = false;
			}
		}
	}
}
//======================================================================================
void Dungeon::Update() {
	UpdateMovementState();
	exploreAroundPlayer();
	updateMechanisms();
	UpdateMonsters();
	updateArrows();
}
//======================================================================================
void Dungeon::Move(float dirX, float dirY, bool jump) {
	if (!Game().player->jump.falling && jump && Map(mapX, mapY).type != Ladder) {
		mapY = mapY + dirY;
		mapX = mapX + dirX;
		Game().player->jump.falling = true;
	}

	if (dirX != 0) {
		float halfWidth = static_cast<float>(Game().player->scale / 60.0);
		float probeX = mapX + dirX + (dirX > 0 ? halfWidth : -halfWidth);
		if (!isSolidTile(Map(mapX, mapY)) && !isSolidTile(Map(probeX, mapY)))
			mapX += dirX;
		else if (Map(probeX, mapY).type == Gate)
			bumpGate(static_cast<int>(probeX), static_cast<int>(mapY));
	}

	if (dirY > 0) {
		if (Map(mapX, mapY).type == Ladder &&
			Map(mapX, mapY + dirY + static_cast<float>(Game().player->scale / 40.0)).type == Ladder)
			mapY += dirY;
	} else if (Map(mapX, mapY).type == Ladder && Map(mapX, mapY + dirY).type == Ladder)
		mapY += dirY;

	// Climbing pulls the player over to the ladder, so the hands reach the rungs.
	if (dirY != 0 && Map(mapX, mapY).type == Ladder) {
		float offset = std::floor(mapX) + LADDER_GRIP_X - mapX;
		mapX += std::clamp(offset, -std::fabs(dirY), std::fabs(dirY));
	}
}
//======================================================================================
bool Dungeon::PlayerOnLadder() const {
	if (Map(mapX, mapY).type != Ladder || std::fabs(mapX - std::floor(mapX) - LADDER_GRIP_X) > LADDER_REACH)
		return false;
	bool floorBelow = isSolidTile(Map(mapX, mapY - 1));
	return !floorBelow || mapY - std::floor(mapY) >= STANDING_EPSILON;
}
//======================================================================================
float Dungeon::ClimbPhase() const { return mapY + CLIMB_SIDE_RATE * mapX; }
//======================================================================================
int Dungeon::Type(float x, float y) { return Map(x, y).type; }
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
