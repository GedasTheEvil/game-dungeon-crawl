// The boss fight (Dungeon::bossFight): the boss appears with its minions, summons more while it lives, and its death
// opens the boss gates. docs/plan/boss-rooms.draft.md.
#include "dungeon.h"
#include "../state/game_state.h"
#include "../core/gameplay_config.h"
#include <algorithm>
#include <cmath>

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
int Dungeon::NearestMonsterHealth() const {
	const Monster* nearest = nullptr;
	auto distance = [this](const Monster& mon) {
		return std::fabs(mon.CentreX() - mapX) + std::fabs(static_cast<float>(mon.Row()) - mapY);
	};
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && (!nearest || distance(mon) < distance(*nearest)))
			nearest = &mon;
	return nearest ? nearest->Health() : 0;
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
