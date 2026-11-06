// Trapped chests and tomb curses (plan trapped-chests): a TrappedTreasure's opening rolls its contents and its trap
// (loot.h); the trap ward's chance stops it. The gas cloud is drawn in dungeon_render_effects.cpp.
// The tomb-curse coffins too (plan tomb-curse-containers).
#include "dungeon.h"
#include "player.h"
#include "../level/journal.h"
#include "../level/loot.h"
#include "../level/world_events.h"
#include <algorithm>
#include <cmath>
#include <string>

// Disarm trap (plan heka-surplus): its own roll, after the trap ward's, only when the ward failed.
bool Dungeon::disarmed() {
	return sim.player->stats.heka.Has(SkillId::DisarmTrap) && sim.random->gameplay.percent(DISARM_TRAP_PERCENT);
}
//======================================================================================
void Dungeon::springChestTrap(int cell) {
	Rng& rng = sim.random->gameplay;
	const bool full = rng.percent(TRAPPED_CHEST_FULL_PERCENT);
	const bool warded = rng.percent(sim.player->stats.TrapWardPercent());
	const bool safe = warded || disarmed();
	std::string line = warded ? "Something clicks inside, and nothing happens. The Eye of Horus saw it first."
					   : safe ? "A thread across the lid, a catch under it: I undo the trap before it springs."
							  : fireChestTrap(cell);
	if (!full && sim.player->stats.heka.Has(SkillId::DisarmTrap)) {
		// Never empty with the skill: what it hides is of its depth, rolled now (learnt mid-level counts too).
		const ItemFileId item = fileIdOf(RollCursedMimicLoot(sim.items->Owned(), levelNumber, rng));
		map[cell].attr = item.type;
		map[cell].value = item.id;
	} else if (!full) {
		clearObject(map[cell]);
		containers[cell].emptied = true; // the open container stays
		line += line.empty() ? "Empty. The trap was the treasure." : "\nEmpty. The trap was the treasure.";
	}
	if (!line.empty()) // a tomb curse told its own lines
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
		tombCurse(RollChestCurse(kind, rng), "An execration text on the lid: cursed be he who opens this.");
		return "";
	case ChestTrap::Swarm:
		chestSwarm(cell);
		return "Scarabs pour out of it!";
	}
	return "";
}
//======================================================================================
void Dungeon::tombCurse(Curse c, const char* execration) {
	sim.events->Status("%s", execration); // stacked over the curse's own line (plan message-log)
	sim.player->stats.LayCurse(c, CurseGrade::Lesser, *sim.events);
	sim.events->Note(FieldNote::TombCurses);
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
float Dungeon::tellNear(int cell) const {
	const int col = cell % MAP_WIDTH, row = cell / MAP_WIDTH;
	const float dx = mapX - (static_cast<float>(col) + 0.5f);
	const float dy = mapY - static_cast<float>(row);
	const float d = std::sqrt(dx * dx + dy * dy);
	return std::clamp((CHEST_TELL_FAR - d) / (CHEST_TELL_FAR - CHEST_TELL_NEAR), 0.f, 1.f);
}
//======================================================================================
float Dungeon::chestTell(int cell) const {
	if (map[cell].type != TrappedTreasure || containers[cell].opened || cursedCoffin(cell))
		return 0.f; // a coffin's tell is its text (coffinTell)
	return tellNear(cell);
}
//======================================================================================
// Tomb-curse coffins (plan tomb-curse-containers): the text on the lid is the tell, the same for an inscribed coffin
// and a trapped one. Once open, the lid lies behind it, its text still on it.
bool Dungeon::cursedCoffin(int cell) const {
	const TreasureLook look = treasureLook(cell);
	return look.kind == Container::Coffin && (look.inscribed || map[cell].type == TrappedTreasure);
}
//======================================================================================
float Dungeon::coffinTell(int cell) const { return cursedCoffin(cell) ? tellNear(cell) : 0.f; }
//======================================================================================
// One roll on the gameplay stream, as a chest trap's: the trap ward's chance, then the curse.
void Dungeon::coffinCurse() {
	Rng& rng = sim.random->gameplay;
	if (rng.percent(sim.player->stats.TrapWardPercent())) {
		sim.events->Status("The red text on the lid flakes away as I lift it. The Eye of Horus read it first.");
		return;
	}
	if (disarmed()) { // plan heka-surplus
		sim.events->Status("I read the curse backwards before I lift the lid. It unravels.");
		return;
	}
	tombCurse(
		RollChestCurse(Container::Coffin, rng),
		"An execration text on the lid: cursed be he who disturbs the rest of this one."
	);
}
