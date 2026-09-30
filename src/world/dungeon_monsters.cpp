#include "dungeon.h"
#include "../state/game_state.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include "../graphics/render_config.h"
#include "../core/gameplay_config.h"
#include "loot.h"
#include "../graphics/lighting.h"

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
// The centre of the landing cell. On the player's cell: in reach of them, never past them, or the monster would turn
// and leap back over the gap.
float Dungeon::leapTarget(const Monster& mon, int land, int dir) const {
	const float centre = static_cast<float>(land) + 0.5f;
	if (static_cast<int>(std::floor(mapX)) != land)
		return centre;
	const float stop = mon.HalfWidth() + Game().player->HalfWidth() + MONSTER_BITE_REACH / 2.f;
	const float bite = mapX - static_cast<float>(dir) * stop;
	return dir > 0 ? std::clamp(bite, static_cast<float>(land), centre)
				   : std::clamp(bite, centre, static_cast<float>(land + 1));
}
//======================================================================================
void Dungeon::UpdateMonsters() {
	updateBoss();
	for (Monster& mon : monsters) {
		if (!mon.Active())
			continue;

		if (mon.flies()) {
			if (!Game().hasWon) {
				const auto col = static_cast<int>(std::floor(mon.flightProbeX()));
				mon.Fly(!IsInBounds(col, mon.Row()) || isSolidTile(MapAt(col, mon.Row())), mapX, mapY);
			}
			continue;
		}

		if (mon.jumping()) { // lands even if killed in the air
			mon.UpdateJump();
			continue;
		}

		if (mon.LeavesChest()) { // a treasure tile never spawns a monster again
			ItemFileId loot = fileIdOf(RollMimicLoot(Game().random.gameplay));
			map[MapIndex(mon.Col(), mon.Row())] = Tile{Treasure, loot.type, loot.id};
			mon.Clear();
			continue;
		}

		if (mon.Alive() && !Game().hasWon && mon.Lurk(mapX, mapY))
			continue;

		if (mon.Alive() && !Game().hasWon && mon.StepDue()) {
			int dir = mon.attackDirection(mapX, mapY);
			auto col = static_cast<int>(std::floor(mon.seekProbeX(dir)));
			bool blocked = walkerBlocked(col, mon.Row());
			if (blocked && dir != 0 && mon.canJump()) {
				int land = leapLanding(col, mon.Row(), dir);
				// Not while the player is in the gap: the rat would leap over them.
				float gapFrom = static_cast<float>(dir > 0 ? col : land + 1);
				float gapTo = static_cast<float>(dir > 0 ? land : col + 1);
				if (land >= 0 && (mapX < gapFrom || mapX >= gapTo)) {
					mon.Jump(leapTarget(mon, land, dir) - static_cast<float>(mon.Col()) - 0.5f);
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
int Dungeon::MonsterBarsShown() const {
	int n = 0;
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && mon.Alerted())
			n++;
	return n;
}
//======================================================================================
void Dungeon::clearMonsters() {
	for (Monster& mon : monsters)
		mon.Clear();
	arrows.clear();
	bossFight = BossFight{};
}
//======================================================================================
// A monster walks away from its spawn tile, so it is culled by where it is now.
bool Dungeon::inView(const Monster& mon) const { return mon.Active() && view().contains(mon.CentreX(), mon.Row()); }
//======================================================================================
// Every active monster, seen or not: a dead one finishes its death clip (a killed mimic leaves its chest) off screen
// too. The blood has its own random stream, so this does not change the game's rolls.
void Dungeon::AnimateMonsters() {
	for (Monster& mon : monsters)
		if (mon.Active())
			mon.Animate(mapX, mapY);
}
//======================================================================================
// Called in Draw() with the frame origin at the view's first column (ViewWindow::firstCol).
void Dungeon::DrawMonsters() {
	const int firstCol = view().firstCol();
	const int firstRow = view().originRow;
	for (Monster& mon : monsters) {
		if (!inView(mon))
			continue;

		glPushMatrix();
		glTranslatef(RenderConfig::TILE_SIZE * static_cast<float>(mon.Col() - firstCol),
					 RenderConfig::TILE_SIZE * static_cast<float>(mon.Row() - firstRow), 0);
		glTranslatef(RenderConfig::MONSTER_OFFSET_X, 0, RenderConfig::MONSTER_OFFSET_Z);
		mon.Draw();
		glPopMatrix();
	}
}
//======================================================================================
bool Dungeon::AttackNearest(int damage, float reach, int dir) {
	Monster* nearest = nullptr;
	for (Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && mon.Nearby(mapX, mapY, reach, dir) &&
			(!nearest || mon.MeleeGap(mapX, dir) < nearest->MeleeGap(mapX, dir)))
			nearest = &mon;
	if (!nearest)
		return false;
	nearest->takeHit(damage);
	return true;
}
//======================================================================================
// A monster tile came into view: its monster appears, unless it is already there. Uses a free slot, else the slot
// of a dead monster.
bool Dungeon::SpawnMonster(int i, int j) {
	const int typeId = Map(static_cast<float>(i), static_cast<float>(j)).attr;
	if (typeId < 1 || typeId > MONSTER_TYPE_MAX)
		return false;
	for (const Monster& mon : monsters)
		if (mon.Active() && !mon.Minion() && mon.Col() == i && mon.Row() == j)
			return false;
	Monster* slot = freeMonsterSlot();
	if (!slot)
		return false;
	slot->Spawn(Game().assets.monsterTypes[typeId], i, j);
	if (slot->Type()->isBoss())
		startBossFight(static_cast<int>(slot - monsters));
	return true;
}
//======================================================================================
// Never the boss's slot: its death is handled on the next update (updateBoss).
Monster* Dungeon::freeMonsterSlot() {
	for (Monster& mon : monsters)
		if (!mon.Active())
			return &mon;
	for (Monster& mon : monsters)
		if (mon.Health() < 1 && &mon - monsters != bossFight.slot)
			return &mon;
	return nullptr;
}
