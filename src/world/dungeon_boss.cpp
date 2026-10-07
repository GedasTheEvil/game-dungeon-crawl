// The boss fight (Dungeon::bossFight): the boss appears with its minions, summons more while it lives, and its death
// opens the boss gates. docs/plan/solved/boss-rooms.md.
#include "dungeon.h"
#include "../state/assets.h"
#include "../entities/player.h"
#include "item_bag.h"
#include "journal.h"
#include "rng.h"
#include "world_events.h"
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
void Dungeon::HurtBoss(int dmg) {
	if (bossFight.slot >= 0)
		playerHit(monsters[bossFight.slot], dmg, nullptr);
}
//======================================================================================
void Dungeon::PoisonBoss(PoisonTier tier) {
	if (bossFight.slot >= 0)
		monsters[bossFight.slot].TakePoison(tier, true, sim.random->gameplay);
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
const Monster* Dungeon::nearestMonster() const {
	const Monster* nearest = nullptr;
	auto distance = [this](const Monster& mon) {
		return std::fabs(mon.CentreX() - mapX) + std::fabs(static_cast<float>(mon.Row()) - mapY);
	};
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && (!nearest || distance(mon) < distance(*nearest)))
			nearest = &mon;
	return nearest;
}
//======================================================================================
int Dungeon::NearestMonsterHealth() const {
	const Monster* nearest = nearestMonster();
	return nearest ? nearest->Health() : 0;
}
//======================================================================================
int Dungeon::NearestMonsterPoison() const {
	const Monster* nearest = nearestMonster();
	return nearest ? nearest->PoisonMask() : 0;
}
//======================================================================================
void Dungeon::PoisonNearestMonster(PoisonTier tier) {
	if (const Monster* nearest = nearestMonster())
		monsters[nearest - monsters].TakePoison(tier, true, sim.random->gameplay);
}
//======================================================================================
void Dungeon::startBossFight(int slot) {
	bossFight = BossFight{slot, 0, 0};
	const Monster& boss = monsters[slot];
	const BossRules& rules = boss.Type()->boss;
	bossFight.nextSummonMs = GameClock::now() + rules.summonMs;
	if (rules.summon == Summon::Hatch) {
		bossFight.firstLeft = rules.minAlive; // on the next updates, once the egg clusters are in play
		return;
	}
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
		// It does not come back, not after a load either; its coffins stay.
		Tile& spawn = map[MapIndex(boss.Col(), boss.Row())];
		setObject(spawn, slainBossObject(spawn.attr));
		bossFight = BossFight{};
		openGates(BOSS_LOCK);
		sim.events->Status("%s", "The guardian is slain!\nThe boss gate grinds open");
		return;
	}
	const BossRules& rules = boss.Type()->boss;
	while (bossFight.firstLeft > 0 && summonMinion(boss))
		bossFight.firstLeft--;
	int now = GameClock::now();
	if (!boss.Alerted() || won || !sim.player->Alive() || now < bossFight.nextSummonMs)
		return;
	bossFight.nextSummonMs = now + rules.summonMs;
	if (bossFight.summoned < rules.summonCap && LivingMinions() < rules.maxAlive && summonMinion(boss))
		bossFight.summoned++;
}
//======================================================================================
// Next to the boss on its row, on the side away from the player (never behind them): the nearest cell a minion can
// stand in with no monster in it yet, else the boss's own cell. A minion summoned into a coffin takes the free coffin
// nearest the boss, not behind the player and not beside them (on its row); with none free there is no summon.
bool Dungeon::summonMinion(const Monster& boss) {
	const MonsterType& kind = sim.assets->monsterTypes[boss.Type()->boss.minion];
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
	const Summon how = boss.Type()->boss.summon;
	if (how == Summon::Coffin) {
		const auto playerCol = static_cast<int>(std::floor(mapX));
		const bool playerHere = std::fabs(mapY - static_cast<float>(row)) < 0.5f; // not on its way in from elsewhere
		int best = -1;
		for (int c = 0; c < MAP_WIDTH; c++) {
			const bool besidePlayer = playerHere && std::abs(c - playerCol) <= 1;
			const bool beyondPlayer = playerHere && (c - playerCol) * (bossCol - playerCol) < 0; // they are between
			if (!bossCoffin(c, row) || besidePlayer || beyondPlayer || taken(c))
				continue;
			if (best < 0 || std::abs(c - bossCol) < std::abs(best - bossCol))
				best = c;
		}
		Monster* slot = best >= 0 ? freeMonsterSlot() : nullptr;
		if (slot == nullptr)
			return false;
		slot->Spawn(kind, best, row, monsterLinks(), sim.random->effects);
		slot->MakeMinion(how);
		sim.journal->SeeMove(boss.Type()->id, levelNumber, CreatureMove::Summon);
		return true;
	}
	if (how == Summon::Hatch) {
		const Monster* best = nullptr; // the living egg cluster nearest the boss, not right by the player
		for (const Monster& mon : monsters) {
			if (!mon.Active() || !mon.Alive() || mon.Type()->id != boss.Type()->boss.nest ||
				(std::fabs(mon.CentreX() - mapX) < 1.5f && std::fabs(static_cast<float>(mon.Row()) - mapY) < 0.5f))
				continue;
			auto distance = [&boss](const Monster& m) {
				return std::fabs(m.CentreX() - boss.CentreX()) + std::fabs(static_cast<float>(m.Row() - boss.Row()));
			};
			if (!best || distance(mon) < distance(*best))
				best = &mon;
		}
		Monster* slot = best ? freeMonsterSlot() : nullptr;
		if (slot == nullptr)
			return false;
		slot->Spawn(kind, best->Col(), best->Row(), monsterLinks(), sim.random->effects);
		slot->MakeMinion(how);
		sim.events->Play(WorldSound::SummonDig);
		sim.journal->SeeMove(boss.Type()->id, levelNumber, CreatureMove::Summon);
		return true;
	}
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
	slot->Spawn(kind, col, row, monsterLinks(), sim.random->effects);
	slot->MakeMinion(how);
	sim.events->Play(how == Summon::Drop ? WorldSound::SummonDrop : WorldSound::SummonDig);
	sim.journal->SeeMove(boss.Type()->id, levelNumber, CreatureMove::Summon);
	return true;
}
