#ifndef CURSES_H
#define CURSES_H

#include <array>
#include <cstdint>
#include <istream>
#include <ostream>

// Curses (docs/plan/solved/curses.md): timed debuffs a monster's hit lays on the player. Several run at once; the same curse
// again restarts its time (no stacking). No cure, no resistance: they run out, or end on death.

// Saved by place: append only, never reorder. Cells of the HUD icon atlas from PlayerHud::Icon::FirstCurse on.
enum class Curse : std::uint8_t { Weakness, Clumsiness, Vulnerability, Fatigue, Disease, Slowness, Palsy, Insanity };
constexpr int CURSE_COUNT = 8;

// A greater curse is its lesser one at a stronger grade (like the poison tiers). Not every curse has one.
enum class CurseGrade : std::uint8_t { Lesser, Greater };

constexpr int CURSE_MS = 120000; // every curse lasts 2 minutes

// The effects, in percent (PlayerStats applies them).
constexpr int WEAKNESS_MIGHT_CUT = 50;			   // of Might
constexpr int GREATER_WEAKNESS_MIGHT_CUT = 125;	   // Might goes negative
constexpr int CLUMSINESS_DAMAGE_CUT = 20;		   // of the whole hit (Might and weapon), not the venom
constexpr int GREATER_CLUMSINESS_DAMAGE_CUT = 50;  //
constexpr int VULNERABILITY_ARMOR_CUT = 60;		   // of the armour
constexpr int FATIGUE_STAMINA_PERCENT = 20;		   // of max stamina at the start, at most; no regeneration
constexpr int DISEASE_MAX_HP_CUT = 30;			   // of max HP
constexpr int GREATER_DISEASE_MAX_HP_CUT = 70;	   //
constexpr float SLOWNESS_WALK_FACTOR = 0.5f;	   // walk and climb: 1 -> 0.5 tiles/s
constexpr float SLOWNESS_SPRINT_MULTIPLIER = 2.5f; // of the slowed walk: 3 -> 1.25 tiles/s
constexpr float SPRINT_MULTIPLIER = 3.f;		   // of the walk, uncursed

struct CurseDef {
	const char* name;		 // the HUD's and the journal's ("Weakness")
	const char* greaterName; // nullptr: no greater grade
	const char* startLine;	 // the status line when it takes
	const char* againLine;	 // ... when it takes again while running
	const char* endLine;	 // ... when it runs out
};
extern const std::array<CurseDef, CURSE_COUNT> CURSES;
[[nodiscard]] inline const CurseDef& curseDef(Curse c) { return CURSES[static_cast<size_t>(c)]; }

// A monster's chance to lay a curse with a hit that lands (MonsterKind::meleeCurses, rangedCurses). percent 0: none.
struct CurseEntry {
	Curse kind = Curse::Weakness;
	CurseGrade grade = CurseGrade::Lesser;
	int percent = 0;
};
// At most two per trigger, the strongest first: a hit rolls them in order, the first that takes is laid (one curse a
// hit).
using CurseEntries = std::array<CurseEntry, 2>;

// The curses running on the player. Saved with PlayerStats.
class Curses {
  public:
	// Lays it for CURSE_MS. A greater grade over a running lesser one raises it; a lesser one over a running greater
	// one only restarts the time. True when it was not running.
	bool Lay(Curse c, CurseGrade grade);
	[[nodiscard]] bool Has(Curse c) const { return LeftMs(c) > 0; }
	[[nodiscard]] int LeftMs(Curse c) const { return running[static_cast<size_t>(c)].leftMs; }
	[[nodiscard]] CurseGrade Grade(Curse c) const { return running[static_cast<size_t>(c)].grade; }
	[[nodiscard]] bool Greater(Curse c) const { return Has(c) && Grade(c) == CurseGrade::Greater; }
	[[nodiscard]] bool Any() const;
	// Time passes; returns a bit per curse that ran out.
	unsigned Advance(int ms);
	void EndAll() { running = {}; }

	// "CURSES n kind grade ms ..."; LoadDump reads it when the tag is there (saves from before the curses have none).
	void Dump(std::ostream& out) const;
	void LoadDump(std::istream& in);

  private:
	struct Running {
		CurseGrade grade = CurseGrade::Lesser;
		int leftMs = 0;
	};
	std::array<Running, CURSE_COUNT> running{};
};

// The might, armour and max HP left under the running curses (at least 0 for armour and max HP, at least 1 max HP).
[[nodiscard]] int cursedMight(int might, const Curses& curses);
[[nodiscard]] int cursedArmor(int armor, const Curses& curses);
[[nodiscard]] int cursedMaxHP(int maxHp, const Curses& curses);
// A hit's damage (Might and weapon) under clumsiness, at least 1.
[[nodiscard]] int cursedHit(int damage, const Curses& curses);

#endif
