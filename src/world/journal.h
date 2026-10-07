#ifndef JOURNAL_H
#define JOURNAL_H

#include "damage.h"
#include "items.h"
#include <istream>
#include <ostream>
#include <string>
#include <vector>

// The archaeologist's notebook (docs/plan/solved/journal-sections.md): what the player went through, kept per save
// game: creatures, riddles and field notes.

// A riddle met at a gate. A copy, not an index into riddles/*.txt: those files can change between saves.
struct JournalRiddle {
	std::string theme;
	std::vector<std::string> question; // one entry per Q: line
	std::vector<std::string> answers;  // normalized (NormalizeAnswer)
	std::string hint;
	int level = 0;			// campaign level of the gate
	int lateXP = 0;			// a late answer from the journal; fixed when the gate is met
	bool hintShown = false; // the hint is written down only once it was shown
	bool solved = false;
	bool solvedLate = false; // answered from the journal, not at the gate
	std::string written;	 // the answer as the player typed it

	[[nodiscard]] bool SameRiddle(const JournalRiddle& other) const {
		return theme == other.theme && question == other.question;
	}
};

// Special moves a monster type can be seen doing, written down the first time.
enum class CreatureMove : unsigned char {
	Leap,	 // a walk-jumper's leap
	Swoop,	 // a bat drops from the ceiling
	Ambush,	 // the mimic's chest opens
	Rise,	 // the mummy climbs out of its coffin
	Summon,	 // a boss calls its minions
	Heal,	 // a life-stealing bite
	Surface, // the crocodile comes up out of the water
	Poison,	 // a poisoned bite or sting
	Rear,	 // the cobra rears up out of its coil
	Spit,	 // venom spat from afar
	Burrow,	 // Apep dives into the floor and comes up elsewhere
	Charge,	 // Sobek charges along the row
};
constexpr int CREATURE_MOVE_COUNT = 12;

// What the archaeologist knows about one monster type (MonsterTypeId). Every note comes from a meeting, never from
// a kill count (docs/plan/solved/monster-journal.md).
struct JournalCreature {
	int type = 0;
	int level = 0;		 // campaign level of the first meeting
	bool killed = false; // name, description and HP
	bool hitBy = false;	 // how hard it hits
	unsigned moves = 0;	 // CreatureMove bits
	// DamageType bits: hit with a weapon of that main type, its resistance is known. TRIED_POISON: the venom amulet's
	// poison tried on it, its poison resistance is known.
	unsigned tried = 0;
	static constexpr unsigned TRIED_POISON = 1U << DAMAGE_TYPE_COUNT;
	[[nodiscard]] bool Saw(CreatureMove m) const { return (moves & (1U << static_cast<unsigned>(m))) != 0; }
	[[nodiscard]] bool Tried(DamageType t) const { return (tried & (1U << static_cast<unsigned>(t))) != 0; }
	[[nodiscard]] bool TriedPoison() const { return (tried & TRIED_POISON) != 0; }
};

// Field notes: the game's rules in the archaeologist's words, each written the first time it matters. A note covers
// only what the player has met: one per potion group, written when the first potion of it is picked up, and one per
// damage type, written when the first weapon of that main type is found (docs/plan/journal-notes-per-kind.md).
enum class FieldNote : unsigned char {
	Health,			// the first hit taken
	Levels,			// the first level up
	Stamina,		// the first jump or sprint without the stamina for it
	Keys,			// the first key picked up
	Poison,			// the first time poisoned
	Weapons,		// the first weapon found: the weapon keys, copies and upgrades
	Blunt,			// the first weapon found that deals mostly blunt damage (weaponMix)
	Slash,			// ... slash
	Pierce,			// ... pierce
	HealthPotions,	// the first small or large health potion picked up
	StaminaPotions, // the first small or large stamina potion
	Might,			// the first Aphethamine
	Armor,			// the first Stone Skin
	Life,			// the first Elixir of Life
	Antidote,		// the first antidote
};
constexpr int FIELD_NOTE_COUNT = 15;
// The note of a damage type, of a potion's group.
[[nodiscard]] FieldNote damageNote(DamageType type);
[[nodiscard]] FieldNote potionNote(ItemKind potion);

// One tenth of the gate's reward (riddleXP), at least 50.
[[nodiscard]] int lateRiddleXP(int gateXP);

class Journal {
  public:
	// A riddle asked at a gate. A riddle met again keeps its entry (the late XP and the level of the first time).
	// Returns its index in Riddles().
	size_t MeetRiddle(const JournalRiddle& riddle);
	void ShowHint(size_t index) { riddles.at(index).hintShown = true; }
	void Solve(size_t index, bool late, const std::string& typed);
	[[nodiscard]] const std::vector<JournalRiddle>& Riddles() const { return riddles; }

	// A monster seen on screen; each of the others sees it too. Creatures keep the order they were first seen in.
	void SeeCreature(int type, int level) { creature(type, level); }
	void KillCreature(int type, int level) { creature(type, level).killed = true; }
	void HitByCreature(int type, int level) { creature(type, level).hitBy = true; }
	void SeeMove(int type, int level, CreatureMove move) {
		creature(type, level).moves |= 1U << static_cast<unsigned>(move);
	}
	// Hit with a weapon of this main type. True the first time: a new note.
	bool TryDamage(int type, int level, DamageType damage);
	// Poisoned by the player (it took or not). True the first time: a new note.
	bool TryPoison(int type, int level);
	[[nodiscard]] const std::vector<JournalCreature>& Creatures() const { return creatures; }

	// Written once; the notes keep the order they were learnt in.
	void LearnNote(FieldNote note);
	[[nodiscard]] const std::vector<FieldNote>& Notes() const { return notes; }

	void Clear() {
		riddles.clear();
		creatures.clear();
		notes.clear();
	}

	// Saved after the dungeon. Saves from before the journal end before the tag: the journal stays empty.
	void Dump(std::ostream& out) const;
	void Load(std::istream& in);

  private:
	std::vector<JournalRiddle> riddles;
	std::vector<JournalCreature> creatures;
	std::vector<FieldNote> notes;

	JournalCreature& creature(int type, int level);
};

#endif
