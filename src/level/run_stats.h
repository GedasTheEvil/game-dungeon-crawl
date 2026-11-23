#ifndef RUN_STATS_H
#define RUN_STATS_H

#include "items.h"
#include "level.h"
#include <array>
#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <vector>
#include <utility>

// How the run ended (plan ending-choice): the ankh taken, life without end; or the gods renounced as demons and the
// ankh broken. Both win the campaign. Chosen at the ankh (Dungeon::Win), recorded in the Hall of Records.
enum class RunEnding : std::uint8_t { Immortal, Renounced };
[[nodiscard]] constexpr const char* endingName(RunEnding e) {
	return e == RunEnding::Renounced ? "Renounced the gods" : "Immortal";
}

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
	int secrets = 0; // name gates opened, each once (plan leaderboard-secrets)
	int deaths = 0; // counted at the death, also written into the autosave then (plan autosave)
	// Picks each level's royal name and its cartouche picture (plan secret-name-gates, names.h); New Game rolls it.
	uint32_t nameSeed = 0;
	// The first hints shown this run (plan controls-help), a bit per FirstHint (hintBit): each shows once a run.
	uint32_t hints = 0;
	// Offerings made at the shrines this run, merges not counted (plan offering-gift): with nameSeed, the seed of the
	// next offering's gift roll, so a quick load gives the same gift again.
	int offerings = 0;
	// Set at the win (plan ending-choice). Not in the save: a won game is never saved.
	RunEnding ending = RunEnding::Immortal;

	void Slay(int monsterType);
	void Drink(ItemKind potion);
	// A hit of the player's that took `dealt` HP, with `weapon` (plan endgame-stats-skills-weapon: per weapon kind;
	// none for a hit of no weapon's, a cheat or a test).
	void Hit(int dealt, std::optional<ItemKind> weapon = std::nullopt);
	void AddPlayMs(int ms, int level); // a tick in play on the campaign level
	[[nodiscard]] int Slain(int monsterType) const { return slain.at(static_cast<size_t>(monsterType)); }
	[[nodiscard]] int Drunk(ItemKind potion) const { return drunk.at(std::to_underlying(potion)); }
	[[nodiscard]] int SlainTotal() const;
	[[nodiscard]] int DrunkTotal() const;
	[[nodiscard]] int WeaponDealt(ItemKind weapon) const { return weaponDealt.at(std::to_underlying(weapon)); }
	// The favourite weapon: the one whose hits took the most HP (the first kind on a tie); none if no weapon hit.
	[[nodiscard]] std::optional<ItemKind> FavoriteWeapon() const;
	[[nodiscard]] int PlayMs() const;
	[[nodiscard]] const std::vector<int>& LevelMs() const { return levelMs; } // [level - 1]: time in play there

	// "RUN2 ..." (drunk by file id; the old "RUN" has it by ItemKind position), "NAMES seed", "DEATHS n", "SECRETS n",
	// "HINTS bits", "OFFERINGS n" and "WEAPONS n type id dealt..." lines after the player stats; a save without them
	// (or broken) leaves a fresh, non-campaign run (seed 0, no deaths, no secrets, no hints shown, no offerings, no
	// weapon damage).
	void Dump(std::ostream& out) const;
	void LoadDump(std::istream& in);
	// A save game's text with its "DEATHS" line set to `count` (plan autosave: a death rewrites the autosave's count,
	// the rest stays). An old save without the line gets one after its "NAMES" line; false if it has neither.
	[[nodiscard]] static bool SetDeaths(std::string& save, int count);

  private:
	std::array<int, MONSTER_TYPE_MAX + 1> slain{}; // by MonsterTypeId
	std::array<int, ITEM_KIND_COUNT> drunk{}; // by ItemKind, the potions only
	std::array<int, ITEM_KIND_COUNT> weaponDealt{}; // by ItemKind, the weapons only: HP their hits took
	std::vector<int> levelMs;
};

#endif
