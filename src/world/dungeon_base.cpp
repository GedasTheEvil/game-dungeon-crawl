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
		// Climbing steps leave float drift (38.9999 for 39): without the snap a jump off the ladder lands a hair
		// below the floor top, falls and ends up inside the floor tile.
		float nearestRow = std::round(mapY);
		if (std::fabs(mapY - nearestRow) < FALL_START_THRESHOLD)
			mapY = nearestRow;
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
	noteSeenMonsters();
	updateArrows();
	updateTraps();
	spawnInView();
	updateAnimations();
}
//======================================================================================
void Dungeon::spawnInView() {
	const ViewWindow v = view();
	for (int j = v.originRow; j < v.originRow + ViewWindow::HEIGHT; j++)
		for (int i = v.firstCol(); i < v.firstCol() + ViewWindow::WIDTH; i++)
			if (IsInBounds(i, j) && MapAt(i, j).type == MonsterSpawn)
				SpawnMonster(i, j);
}
//======================================================================================
void Dungeon::updateAnimations() {
	if (portalTimer.TimePassed())
		portalScroll -= 0.022f;
	riddleMarkYaw += 1.f;
	treasureSpin += 1.f;
}
//======================================================================================
bool Dungeon::inTrap(float x, float y) const {
	const auto col = static_cast<int>(std::floor(x));
	const auto row = static_cast<int>(std::floor(y));
	for (int j = row - 1; j <= row + 1; j++) // the death trap's hitbox reaches into the next cells
		for (int i = col - 1; i <= col + 1; i++) {
			if (!IsInBounds(i, j))
				continue;
			const int type = MapAt(i, j).type;
			if (type != Spike && type != Death)
				continue;
			const float scale = type == Death ? DEATH_TRAP_SCALE : SPIKES_SCALE;
			if (std::fabs(x - static_cast<float>(i) - 0.5f) <= TRAP_HITBOX_X_SCALE * scale &&
				std::fabs(y - static_cast<float>(j)) <= TRAP_HITBOX_Y_SCALE * scale)
				return true;
		}
	return false;
}
//======================================================================================
// Only walkers on the ground stand in a trap: flyers pass over, a leaper is in the air.
void Dungeon::updateTraps() {
	if (inTrap(mapX, mapY))
		if (const int dmg = trapHurt.hit(); dmg > 0)
			Game().player->TakeHit(dmg);
	for (Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && !mon.flies() && !mon.jumping() &&
			inTrap(mon.CentreX(), static_cast<float>(mon.Row())))
			mon.StandInTrap();
}
//======================================================================================
void Dungeon::Move(float dirX, float dirY) {
	if (dirX != 0) {
		float probeX = mapX + dirX + (dirX > 0 ? PLAYER_BODY_HALF_WIDTH : -PLAYER_BODY_HALF_WIDTH);
		if (!isSolidTile(Map(mapX, mapY)) && !isSolidTile(Map(probeX, mapY)))
			mapX += dirX;
		else if (Map(probeX, mapY).type == Gate)
			bumpGate(static_cast<int>(probeX), static_cast<int>(mapY));
	}

	if (dirY > 0) {
		if (Map(mapX, mapY).type == Ladder && Map(mapX, mapY + dirY + PLAYER_CLIMB_HEADROOM).type == Ladder)
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
