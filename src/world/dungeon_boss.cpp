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
#include "../graphics/fire.h"
#include "../graphics/render_config.h"
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
		// It does not come back, not after a load either; its coffins stay.
		Tile& spawn = map[MapIndex(boss.Col(), boss.Row())];
		setObject(spawn, slainBossObject(spawn.attr));
		bossFight = BossFight{};
		openGates(BOSS_LOCK);
		sim.events->Status("%s", "The guardian is slain!\nThe boss gate grinds open");
		return;
	}
	const BossRules& rules = boss.Type()->boss;
	int now = GameClock::now();
	if (!boss.Alerted() || won || !sim.player->Alive() || now < bossFight.nextSummonMs)
		return;
	bossFight.nextSummonMs = now + rules.summonMs;
	if (bossFight.summoned < rules.summonCap && LivingMinions() < rules.maxAlive && summonMinion(boss))
		bossFight.summoned++;
}
//======================================================================================
// Where a minion came out, while its summon lasts: sand thrown up from the floor or grit falling from the ceiling.
// Same window and frame as drawMechanismEffects.
void Dungeon::drawSummonEffects() {
	const int now = GameClock::now();
	for (const Monster& mon : monsters) {
		const int age = now - mon.SummonedMs();
		if (!mon.Active() || mon.SummonedMs() < 0 || mon.SummonedBy() == Summon::Coffin || age >= Grit::BURST_MS)
			continue;
		const float x = (mon.CentreX() - static_cast<float>(view().originCol)) * RenderConfig::TILE_SIZE;
		const bool drop = mon.SummonedBy() == Summon::Drop;
		const float y = static_cast<float>(mon.Row() - view().originRow + (drop ? 1 : 0)) * RenderConfig::TILE_SIZE;
		Grit::burst(x, y, RenderConfig::MONSTER_DEPTH, age, drop, static_cast<uint32_t>(mon.SummonedMs() + mon.Col()));
	}
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
