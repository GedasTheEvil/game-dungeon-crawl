#include "dungeon.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include <GL/gl.h>
#include <cmath>
#include <memory>
#include "../graphics/render_config.h"
#include "../input/gameplay_config.h"

bool Dungeon::walkerBlocked(int col, int row) const {
	if (!IsInBounds(col, row))
		return true;
	Tile cell = MapAt(col, row);
	if (isSolidTile(cell) || cell.type == Spike || cell.type == Death)
		return true;
	return !IsInBounds(col, row - 1) || !isSolidTile(MapAt(col, row - 1)); // row 0 is the bottom
}
//======================================================================================
int Dungeon::leapLanding(int col, int row, int dir) const {
	if (!IsInBounds(col, row) || isSolidTile(MapAt(col, row)))
		return -1; // a wall, not a gap
	for (int k = 1; k <= MONSTER_JUMP_MAX_GAP; k++) {
		int c = col + dir * k;
		if (!IsInBounds(c, row) || isSolidTile(MapAt(c, row)))
			return -1;
		if (!walkerBlocked(c, row))
			return c;
	}
	return -1;
}
//======================================================================================
void Dungeon::UpdateMonsters() {
	for (Monster& mon : monsters) {
		if (!mon.Active())
			continue;

		if (mon.flies()) {
			if (!GAME_STATE.hasWon) {
				const auto col = static_cast<int>(std::floor(mon.flightProbeX()));
				mon.Fly(!IsInBounds(col, mon.Row()) || isSolidTile(MapAt(col, mon.Row())), mapX, mapY);
			}
			continue;
		}

		if (mon.jumping()) { // lands even if killed in the air
			mon.UpdateJump();
			continue;
		}

		if (mon.Alive() && !GAME_STATE.hasWon && mon.StepDue()) {
			int dir = mon.attackDirection(mapX, mapY);
			auto col = static_cast<int>(std::floor(mon.seekProbeX(dir)));
			bool blocked = walkerBlocked(col, mon.Row());
			if (blocked && dir != 0 && mon.canJump()) {
				int land = leapLanding(col, mon.Row(), dir);
				// Not while the player is in the gap: the rat would leap over them.
				float gapFrom = static_cast<float>(dir > 0 ? col : land + 1);
				float gapTo = static_cast<float>(dir > 0 ? land : col + 1);
				if (land >= 0 && (mapX < gapFrom || mapX >= gapTo)) {
					mon.Jump(static_cast<float>(land) - static_cast<float>(mon.Col()));
					continue;
				}
			}
			if (!mon.Seek(blocked, mapX, mapY))
				if (mon.AttackDue())
					mon.Attack(mapY);
		}
	}
}
//======================================================================================
void Dungeon::clearMonsters() {
	for (Monster& mon : monsters)
		mon.Clear();
}
//======================================================================================
// Called in Draw() with the frame origin at the first drawn tile: column mapX - 4, row mapY - 3.
// A monster walks away from its spawn tile, so it is culled and placed by where it is now.
void Dungeon::DrawMonsters() {
	int firstCol = static_cast<int>(mapX) - 4;
	int firstRow = static_cast<int>(mapY) - 3;
	for (Monster& mon : monsters) {
		if (!mon.Active())
			continue;
		float centre = mon.CentreX(); // the drawn tiles: 10 x 6
		if (mon.Row() < firstRow || mon.Row() >= firstRow + 6 || centre < static_cast<float>(firstCol) ||
			centre >= static_cast<float>(firstCol + 10))
			continue;

		glPushMatrix();
		glTranslatef(RenderConfig::TILE_SIZE * static_cast<float>(mon.Col() - firstCol),
					 RenderConfig::TILE_SIZE * static_cast<float>(mon.Row() - firstRow), 0);
		glTranslatef(RenderConfig::MONSTER_OFFSET_X, 0, RenderConfig::MONSTER_OFFSET_Z);
		mon.Draw(mapX, mapY);
		glPopMatrix();
	}
}
//======================================================================================
void Dungeon::AttackNearest(int damage, int attackRange) {
	for (Monster& mon : monsters) {
		if (!mon.Active())
			continue;
		if (mon.Alive() && mon.Nearby(mapX, mapY, attackRange)) {
			mon.takeHit(damage);
			break;
		}
	}
}
//======================================================================================
// A monster tile came into view: its monster appears, unless it is already there. Uses a free slot, else the slot
// of a dead monster.
bool Dungeon::SpawnMonster(int i, int j) {
	const int typeId = Map(static_cast<float>(i), static_cast<float>(j)).attr;
	if (typeId < 1 || typeId > MONSTER_TYPE_MAX)
		return false;
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Col() == i && mon.Row() == j)
			return false;
	Monster* slot = nullptr;
	for (Monster& mon : monsters)
		if (!mon.Active()) {
			slot = &mon;
			break;
		}
	if (!slot)
		for (Monster& mon : monsters)
			if (mon.Health() < 1) {
				slot = &mon;
				break;
			}
	if (!slot)
		return false;
	slot->Spawn(GAME_STATE.monsterTypes[typeId], i, j);
	return true;
}
