#ifndef JOURNAL_H
#define JOURNAL_H

#include "curses.h"
#include "damage.h"
#include "inscriptions.h"
#include "items.h"
#include "skills.h"
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

// The archaeologist's notebook (plan journal-sections): what the player went through, kept per save game: creatures,
// riddles and field notes.

// A riddle met at a gate. A copy, not an index into content/riddles/*.txt: those files can change between saves.
struct JournalRiddle {
	std::string theme;
	std::vector<std::string> question; // one entry per Q: line
	std::vector<std::string> answers; // normalized (NormalizeAnswer)
	std::string hint;
	int level = 0; // campaign level of the gate
	int lateXP = 0; // a late answer from the journal; fixed when the gate is met
	bool hintShown = false; // the hint is written down only once it was shown
	bool solved = false;
	bool solvedLate = false; // answered from the journal, not at the gate
	std::string written; // the answer as the player typed it

	[[nodiscard]] bool SameRiddle(const JournalRiddle& other) const {
		return theme == other.theme && question == other.question;
	}
};

// A wall inscription read (plan hieroglyph-cartouches). A copy of its text, like a riddle: content/inscriptions/*.txt
// can change between saves.
struct JournalInscription {
	int id = 0; // the text id (inscriptions.h); its glyphs on the page come from the atlas cell of this id
	std::string title;
	std::vector<std::string> text;
	std::string note;
	int level = 0; // campaign level where it was read first
	// False for a copy made without the hieroglyph skill: the glyphs only, the text stays hidden until the skill is
	// learnt (plan skill-hieroglyphs).
	bool translated = true;
	Boon boon; // given when it is translated (plan inscription-boons)
	// A name cartouche (id NAME_CARTOUCHE_ID, plan secret-name-gates): its cell of the name cartouche atlas; one per
	// level, so the level tells them apart. -1 for any other inscription.
	int picture = -1;

	[[nodiscard]] bool SameInscription(const JournalInscription& other) const {
		return id == other.id && (id != NAME_CARTOUCHE_ID || level == other.level);
	}
};

// Special moves a monster type can be seen doing, written down the first time.
enum class CreatureMove : unsigned char {
	Leap, // a walk-jumper's leap
	Swoop, // a bat drops from the ceiling
	Ambush, // the mimic's chest opens
	Rise, // the mummy climbs out of its coffin
	Summon, // a boss calls its minions
	Heal, // a life-stealing bite
	Surface, // the crocodile comes up out of the water
	Poison, // a poisoned bite or sting
	Rear, // the cobra rears up out of its coil
	AttackRanged, // a ranged attack from afar: venom spat, a bolt shot
	Burrow, // Apep dives into the floor and comes up elsewhere
	Charge, // Sobek and the hippo charge along the row
	Climb, // a climber follows the player up or down a ladder
	Curse, // a hit that lays a curse (MonsterKind::meleeCurses, rangedCurses)
	Bloom, // the blue lotus rears up off the water
	Sedate, // a bite that drains stamina (MonsterKind::staminaDrainPct)
	ShunLight, // a light shunner turns back at the light round the player (MonsterKind::shunsLight)
	Grab, // a bite that grabs and holds the player (MonsterKind::hold)
	StepOff, // a figurine steps off its place among the decor figures (Locomotion::Figurine)
};
constexpr int CREATURE_MOVE_COUNT = 19;

// What the archaeologist knows about one monster type (MonsterTypeId). Every note comes from a meeting, never from a
// kill count (plan monster-journal).
struct JournalCreature {
	int type = 0;
	int level = 0; // campaign level of the first meeting
	bool killed = false; // name, description and HP
	bool hitBy = false; // how hard it hits
	unsigned moves = 0; // CreatureMove bits
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
// damage type, written when the first weapon of that main type is found (plan journal-notes-per-kind).
enum class FieldNote : unsigned char {
	Health, // the first hit taken
	Levels, // the first level up
	Stamina, // the first jump or sprint without the stamina for it
	Keys, // the first key picked up
	Poison, // the first time poisoned
	Weapons, // the first weapon found: the weapon keys, copies and upgrades
	Blunt, // the first weapon found that deals mostly blunt damage (weaponMix)
	Slash, // ... slash
	Pierce, // ... pierce
	HealthPotions, // the first small or large health potion picked up
	StaminaPotions, // the first small or large stamina potion
	Might, // the first Aphethamine
	Armor, // the first Stone Skin
	Life, // the first Elixir of Life
	Antidote, // the first antidote
	DartTraps, // the first dart trap plate the player stepped on
	Resistance, // the first lesser or greater resistance potion
	Heka, // the first boss's heka: the skills
	SerketsFavour, // Serket's favour learnt (written on learning, not on the first poison)
	SharpEye, // Sharp eye learnt (written on learning)
	// The first curse of each kind, in Curse order (curseNote).
	CurseWeakness,
	CurseClumsiness,
	CurseVulnerability,
	CurseFatigue,
	CurseDisease,
	CurseSlowness,
	CursePalsy,
	CurseInsanity,
	Fire, // the first time something burns (plan fire-damage)
	FavourOfHapi, // Favour of Hapi learnt (written on learning)
	BlessingOfIsis, // Blessing of Isis learnt (written on learning)
	WaterBlessing, // the first Blessing of Hapi potion (plan water-blessing-potion)
	CanopicJars, // the first canopic jars opened (plan treasure-containers)
	JumpedBite, // the first bite the player jumped over (plan monster-attack-reach)
};
constexpr int FIELD_NOTE_COUNT = 34;
// The field note written when the skill is learnt, none for a skill without one.
[[nodiscard]] std::optional<FieldNote> skillNote(SkillId skill);
// The note of a damage type, of a potion's group.
[[nodiscard]] FieldNote damageNote(DamageType type);
[[nodiscard]] FieldNote potionNote(ItemKind potion);
[[nodiscard]] constexpr FieldNote curseNote(Curse c) {
	return static_cast<FieldNote>(static_cast<int>(FieldNote::CurseWeakness) + static_cast<int>(c));
}

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

	// Written once, the first time it is read or copied; the entries keep the order they were read in. True the first
	// time, or when a copy gets translated by this call (inscription.translated).
	bool ReadInscription(const JournalInscription& inscription);
	// The entry of this inscription (SameInscription), null before it was read or copied.
	[[nodiscard]] const JournalInscription* FindInscription(const JournalInscription& inscription) const;
	// The hieroglyph skill learnt: every copy shows its text. Returns the boon of each copy translated (none for most),
	// one entry per copy.
	std::vector<Boon> TranslateInscriptions();
	[[nodiscard]] const std::vector<JournalInscription>& Inscriptions() const { return inscriptions; }

	void Clear() {
		riddles.clear();
		creatures.clear();
		notes.clear();
		inscriptions.clear();
	}

	// Saved after the dungeon. Saves from before the journal end before the tag: the journal stays empty.
	void Dump(std::ostream& out) const;
	void Load(std::istream& in);

  private:
	std::vector<JournalRiddle> riddles;
	std::vector<JournalCreature> creatures;
	std::vector<FieldNote> notes;
	std::vector<JournalInscription> inscriptions;

	JournalCreature& creature(int type, int level);
};

#endif
