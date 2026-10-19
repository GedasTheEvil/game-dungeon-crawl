#ifndef RUN_STATS_H
#define RUN_STATS_H

#include "items.h"
#include "level.h"
#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
#include <vector>

// What the run did from New Game on (plan leaderboard): the leaderboard records it at the win. Kept in PlayerStats, so
// it rides in the save game: a load puts the counters back, a fight replayed after a load counts once.
class RunStats {
  public:
	// Started by New Game on level 1. A scenario, a dev flag's level or a later level: false, no leaderboard entry.
	bool campaign = false;
	int inscriptions = 0; // read, each once (the journal's first reading)
	int treasures = 0; // treasure cells picked up
	int riddles = 0; // answered, also late from the journal
	int damageDealt = 0; // HP the player's hits took off monsters
	int damageTaken = 0; // HP monsters' hits and traps took off the player (not poison)
	int biggestHit = 0; // the most HP one hit of the player's took
	int poisoned = 0; // poisonings that took (a warded one does not count)
	int cursed = 0; // curses laid, also one laid again while it runs
	// Picks each level's royal name and its cartouche picture (plan secret-name-gates, names.h); New Game rolls it.
	uint32_t nameSeed = 0;

	void Slay(int monsterType);
	void Drink(ItemKind potion);
	void Hit(int dealt);
	void AddPlayMs(int ms, int level); // a tick in play on the campaign level
	[[nodiscard]] int Slain(int monsterType) const { return slain.at(static_cast<size_t>(monsterType)); }
	[[nodiscard]] int Drunk(ItemKind potion) const { return drunk.at(static_cast<size_t>(potion)); }
	[[nodiscard]] int SlainTotal() const;
	[[nodiscard]] int DrunkTotal() const;
	[[nodiscard]] int PlayMs() const;
	[[nodiscard]] const std::vector<int>& LevelMs() const { return levelMs; } // [level - 1]: time in play there

	// "RUN ..." and "NAMES seed" lines after the player stats; a save without them (or broken) leaves a fresh,
	// non-campaign run (seed 0).
	void Dump(std::ostream& out) const;
	void LoadDump(std::istream& in);

  private:
	std::array<int, MONSTER_TYPE_MAX + 1> slain{}; // by MonsterTypeId
	std::array<int, ITEM_KIND_COUNT> drunk{}; // by ItemKind, the potions only
	std::vector<int> levelMs;
};

#endif
