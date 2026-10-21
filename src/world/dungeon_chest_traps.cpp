// Trapped chests and tomb curses (plan trapped-chests): a TrappedTreasure's opening rolls its contents and its trap
// (loot.h); the trap ward's chance stops it. The gas cloud is drawn in dungeon_render_effects.cpp.
#include "dungeon.h"
#include "../entities/player.h"
#include "journal.h"
#include "loot.h"
#include "world_events.h"
#include <algorithm>
#include <cmath>
#include <string>

void Dungeon::springChestTrap(int cell) {
	Rng& rng = sim.random->gameplay;
	const bool full = rng.percent(TRAPPED_CHEST_FULL_PERCENT);
	const bool warded = rng.percent(sim.player->stats.TrapWardPercent());
	std::string line =
		warded ? "Something clicks inside, and nothing happens. The Eye of Horus saw it first." : fireChestTrap(cell);
	if (!full) {
		clearObject(map[cell]);
		containers[cell].emptied = true; // the open container stays
		line += "\nEmpty. The trap was the treasure.";
	}
	sim.events->Status("%s", line.c_str());
}
//======================================================================================
std::string Dungeon::fireChestTrap(int cell) {
	Rng& rng = sim.random->gameplay;
	const Container kind = treasureLook(cell).kind;
	sim.events->Note(FieldNote::TrappedChests);
	switch (RollChestTrap(kind, levelNumber, rng)) {
	case ChestTrap::Darts:
		sim.events->Play(WorldSound::Dart);
		if (const int dmg = sim.player->stats.TrapDamage(CHEST_DART_DAMAGE); dmg > 0)
			sim.player->TakeHit(dmg, DART_ATTACK_MIX, *sim.events);
		sim.player->Poison(PoisonTier::Medium, *sim.events, rng);
		return "Poison darts spring out at me!";
	case ChestTrap::Gas:
		sim.events->Play(WorldSound::SandHiss);
		sim.player->Poison(PoisonTier::Weak, *sim.events, rng); // the first breath, before the trap's line
		chestGas.push_back({cell, GameClock::now(), GameClock::now() + CHEST_GAS_POISON_MS});
		return "A puff of green gas. Do not breathe it.";
	case ChestTrap::Curse:
		return tombCurse(RollChestCurse(kind, rng), "An execration text on the lid: cursed be he who opens this.");
	case ChestTrap::Swarm:
		chestSwarm(cell);
		return "Scarabs pour out of it!";
	}
	return "";
}
//======================================================================================
std::string Dungeon::tombCurse(Curse c, const char* execration) {
	sim.player->stats.LayCurse(c, CurseGrade::Lesser, *sim.events);
	sim.events->Note(FieldNote::TombCurses);
	return std::string(execration) + "\n" + curseDef(c).startLine;
}
//======================================================================================
// On the chest's row, nearest first, either side: the cells a walker can stand in and no one holds.
void Dungeon::chestSwarm(int cell) {
	const MonsterType& scarab = (*sim.monsterTypes)[MonsterScarab];
	const int col = cell % MAP_WIDTH, row = cell / MAP_WIDTH;
	auto taken = [this, row](int c) {
		return std::any_of(std::begin(monsters), std::end(monsters), [c, row](const Monster& mon) {
			return mon.Active() && mon.Alive() && mon.Row() == row && static_cast<int>(std::floor(mon.CentreX())) == c;
		});
	};
	int dug = 0;
	for (int k = 1; k <= CHEST_SWARM_SIZE + 1 && dug < CHEST_SWARM_SIZE; k++)
		for (int c : {col - k, col + k}) {
			if (dug >= CHEST_SWARM_SIZE || !IsInBounds(c, row) || isSolidTile(MapAt(c, row)) || walkerBlocked(c, row) ||
				taken(c))
				continue;
			Monster* slot = freeMonsterSlot();
			if (slot == nullptr)
				return;
			slot->Spawn(scarab, c, row, monsterLinks(), sim.random->effects);
			slot->MakeMinion(Summon::DigOut);
			dug++;
		}
	if (dug > 0)
		sim.events->Play(WorldSound::SummonDig);
}
//======================================================================================
void Dungeon::updateChestGas() {
	const int now = GameClock::now();
	std::erase_if(chestGas, [now](const ChestGas& g) { return now - g.startMs >= CHEST_GAS_MS; });
	for (ChestGas& g : chestGas) {
		const int col = g.cell % MAP_WIDTH, row = g.cell / MAP_WIDTH;
		const bool inside = std::fabs(mapX - (static_cast<float>(col) + 0.5f)) <= CHEST_GAS_HALF_WIDTH &&
							static_cast<int>(std::floor(mapY + STANDING_EPSILON)) == row;
		if (!inside || now < g.nextPoisonMs || !sim.player->Alive())
			continue;
		sim.player->Poison(PoisonTier::Weak, *sim.events, sim.random->gameplay);
		g.nextPoisonMs = now + CHEST_GAS_POISON_MS;
	}
}
//======================================================================================
float Dungeon::chestTell(int cell) const {
	if (map[cell].type != TrappedTreasure || containers[cell].opened)
		return 0.f;
	const int col = cell % MAP_WIDTH, row = cell / MAP_WIDTH;
	const float dx = mapX - (static_cast<float>(col) + 0.5f);
	const float dy = mapY - static_cast<float>(row);
	const float d = std::sqrt(dx * dx + dy * dy);
	return std::clamp((CHEST_TELL_FAR - d) / (CHEST_TELL_FAR - CHEST_TELL_NEAR), 0.f, 1.f);
}
