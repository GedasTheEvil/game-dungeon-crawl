#include "dungeon.h"
#include "../state/game_state.h"
#include <GL/gl.h>
#include <cmath>
#include <memory>
#include "../graphics/render_config.h"
#include "../core/gameplay_config.h"
#include "loot.h"

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
			LootItem loot = RollMimicLoot();
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
const Monster* Dungeon::Boss() const {
	if (bossFight.slot < 0)
		return nullptr;
	const Monster& boss = monsters[bossFight.slot];
	return boss.Alive() && boss.Alerted() ? &boss : nullptr;
}
//======================================================================================
int Dungeon::BossHealth() const { return bossFight.slot >= 0 ? monsters[bossFight.slot].Health() : 0; }
//======================================================================================
void Dungeon::SlayBoss() {
	if (bossFight.slot >= 0)
		monsters[bossFight.slot].takeHit(monsters[bossFight.slot].Health());
}
//======================================================================================
int Dungeon::LivingMinions() const {
	int n = 0;
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && mon.Minion())
			n++;
	return n;
}
//======================================================================================
void Dungeon::startBossFight(int slot) {
	bossFight = BossFight{slot, 0, 0};
	const Monster& boss = monsters[slot];
	const BossRules& rules = boss.Type()->boss;
	bossFight.nextSummonMs = GameClock::now() + rules.summonMs;
	for (int k = 0; k < rules.minAlive; k++)
		summonMinion(boss);
}
//======================================================================================
void Dungeon::updateBoss() {
	if (bossFight.slot < 0)
		return;
	Monster& boss = monsters[bossFight.slot];
	if (!boss.Active() || !boss.Type()->isBoss()) {
		bossFight = BossFight{};
		return;
	}
	if (!boss.Alive()) {
		map[MapIndex(boss.Col(), boss.Row())] = Tile{Empty, 0, 0}; // it does not come back, not after a load either
		bossFight = BossFight{};
		openGates(BOSS_LOCK);
		Game().ShowStatus("%s", "The guardian is slain!\nThe boss gate grinds open");
		return;
	}
	const BossRules& rules = boss.Type()->boss;
	int now = GameClock::now();
	if (!boss.Alerted() || Game().hasWon || !Game().player->Alive() || now < bossFight.nextSummonMs)
		return;
	bossFight.nextSummonMs = now + rules.summonMs;
	if (bossFight.summoned < rules.summonCap && LivingMinions() < rules.maxAlive && summonMinion(boss))
		bossFight.summoned++;
}
//======================================================================================
// Next to the boss on its row, on the side away from the player (never behind them): the nearest cell a minion can
// stand in with no monster in it yet, else the boss's own cell.
bool Dungeon::summonMinion(const Monster& boss) {
	const MonsterType& kind = Game().assets.monsterTypes[boss.Type()->boss.minion];
	const bool flyer = kind.locomotion == Locomotion::Fly;
	const int row = boss.Row();
	const auto bossCol = static_cast<int>(std::floor(boss.CentreX()));
	const int away = boss.CentreX() < mapX ? -1 : 1;
	auto taken = [this, row](int col) {
		for (const Monster& mon : monsters)
			if (mon.Active() && mon.Alive() && mon.Row() == row && static_cast<int>(std::floor(mon.CentreX())) == col)
				return true;
		return false;
	};
	int col = bossCol;
	for (int k = 1; k <= MINION_SUMMON_REACH; k++) {
		int c = bossCol + away * k;
		if (!IsInBounds(c, row) || isSolidTile(MapAt(c, row)))
			break;
		if ((flyer || !walkerBlocked(c, row)) && !taken(c)) {
			col = c;
			break;
		}
	}
	Monster* slot = freeMonsterSlot();
	if (slot == nullptr)
		return false;
	slot->Spawn(kind, col, row);
	slot->MakeMinion();
	return true;
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
bool Dungeon::AttackNearest(int damage, int attackRange, int dir) {
	for (Monster& mon : monsters) {
		if (!mon.Active())
			continue;
		if (mon.Alive() && mon.Nearby(mapX, mapY, attackRange, dir)) {
			mon.takeHit(damage);
			return true;
		}
	}
	return false;
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
