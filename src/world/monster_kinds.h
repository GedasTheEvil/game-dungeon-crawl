#ifndef MONSTER_KINDS_H
#define MONSTER_KINDS_H

// One row per monster type, everything about it but the loaded model: names, glyph, the checker's threat, stats,
// model and texture paths, resistances, how it moves, poisons, spits and summons. The game loads the model of each
// row into a MonsterType (entities/monster.h); levelgen picks from the rows.

#include "../core/gameplay_config.h"
#include "damage.h"
#include "level.h"
#include "poison.h"
#include "rgb.h"
#include <optional>

// How a monster gets around. Only flyers cross pits; walkers stop at their edge, and at traps unless reckless
// (Courage).
enum class Locomotion : unsigned char {
	Stationary, // rooted to its spawn tile (plant), attacks when the player is next to it
	Ambush,		// rooted like Stationary, idle and still (a treasure chest) until the player comes near, see Lurk;
				// killed, it leaves a real treasure chest on its tile
	Walk,		// follows the player along its row
	Entombed,	// walks, but lies in its coffin (idle) until the player comes near or hits it, then climbs out (Rise)
				// before it acts
	WalkJump,	// walks, leaps over pits and traps (giant rat, see Leap)
	Fly,		// see Flight
	Submerged,	// walks, but lies idle in the water until the player comes near or hits it (the crocodile)
	// Walks, but lies coiled (idle) until the player comes near or hits it, then rears up (Rise) before it acts (the
	// cobra).
	Coiled,
	Burrow, // walks, and now and then dives into the floor and comes up elsewhere on its row (Apep, see Burrow)
};

// How a walker moves through half water (crocodiles-and-flooded-cells). Rooted monsters and flyers do not wade.
enum class Wading : unsigned char {
	Slowed,		// slower, like the player (MonsterKind::waterSpeed, WADE_SPEED_FACTOR unless set)
	Unaffected, // full speed
	Swimmer,	// faster (waterSpeed), floating with its back at the surface (swimLift)
};

// Whether a monster sets foot on a trap. Locomotion says what it can do, courage what it wants to.
enum class Courage : unsigned char {
	// Afraid of traps (spikes, death traps, a rock fall not yet fallen): a walker stops at their edge, a walk-jumper
	// leaps over them.
	Coward,
	// Walks straight through them and takes their damage, cut by MonsterKind::trapDamagePct.
	Reckless,
};

// How a boss's minions arrive: they dig out of the floor or drop from the ceiling (Monster::Emerging), climb out
// of a coffin by the boss (entombed minions, Monster::Rising), or hatch out of a living nest (BossRules::nest;
// emerging like DigOut; none left, no summons).
enum class Summon : unsigned char { DigOut, Drop, Coffin, Hatch };

// A boss summons minions around itself while it lives (Dungeon::updateBoss).
struct BossRules {
	int minion = 0;		  // MonsterTypeId of its minions; 0: not a boss
	int minAlive = 0;	  // minions around it when it appears
	int maxAlive = 0;	  // no summon while this many are alive
	int summonMs = 0;	  // between summons
	int summonCap = 0;	  // summons per fight, after the first minAlive
	int lifeStealPct = 0; // heals this share of the damage it deals
	Summon summon = Summon::DigOut;
	int nest = 0;		 // Summon::Hatch: the MonsterTypeId its minions hatch out of
	int smallMinion = 0; // Summon::Hatch: a smaller kind every other hatch (the scorpion queen's scorpions); 0: none
};

// A spitter's venom: from afar along its row, it stops and spits a glob at the player (Dungeon::Venom). The release
// and the mouth height come from the model's spit clip (tools/blender/models/cobra.py).
struct SpitRules {
	int damage = 0; // on a hit, of the type's attack mix
	PoisonTier poison = PoisonTier::Medium;
	float range = 0.f;	  // tiles between the boxes, at most; nearer than MONSTER_BITE_REACH it bites
	int cooldownMs = 0;	  // from one spit to the next
	float release = 0.5f; // of the spit clip: the glob leaves the mouth
	float mouthY = 0.5f;  // of the reference clip's height: the mouth at the release
};

// Where levelgen puts it: levels of difficulty minDifficulty .. maxDifficulty, picked by weight. Weight 0: never.
struct GenPick {
	int minDifficulty = 0;
	int maxDifficulty = 0;
	int weight = 0;
};

constexpr Rgb RED_BLOOD = {0.7f, 0.1f, 0.1f};

struct MonsterKind {
	MonsterTypeId id{};
	char glyph = 'm';			  // levelcheck --map and the ASCII level sources
	const char* name = "";		  // the game's and the journal's ("Man-eater plant")
	const char* label = "";		  // the editor's list ("giant rat")
	const char* description = ""; // the editor's hint
	const char* note = ""; // the journal's description, in the archaeologist's words, written after the first kill
	float threat = 0.f;	   // one of them in the checker's difficulty score

	// Look: under models/ and sounds/, under textures/ (a giant kin is the same model, bigger and darker).
	const char* model = "";
	const char* texture = "";
	float scale = 1.f;
	float rotA = 180.f; // model yaw facing the camera
	Rgb blood = RED_BLOOD;

	float speed = 1.f;
	int maxHealth = 20;
	int damage = 1;
	int attackMs = 800;					 // between bites
	int xp = 0;							 // gained for the kill
	DamageMix attackMix{};				 // what its bite deals; shared by a model's group
	Resistances resist = NO_RESISTANCES; // how it takes each type of a weapon's damage

	Locomotion locomotion = Locomotion::Walk;
	Courage courage = Courage::Coward;
	int trapDamagePct = 100; // share of a trap's damage it takes (traps ignore armour); 0: immune
	// Presses a dart trap's plate at DART_PLATE_WEIGHT and up (docs/plan/solved/poison-dart-trap.md): 0 flyers, 1 small
	// ones (rats, scarabs), 2-3 large ones, more for bosses. Later traps may want more.
	int weight = 2;
	Wading wading = Wading::Slowed;
	float waterSpeed = WADE_SPEED_FACTOR; // its speed in half water, times its speed on land
	std::optional<PoisonTier> poison;	  // its bite or sting poisons the player
	// Chance that a poisoning does not take (docs/plan/solved/monster-poison.md); 100: immune. A boss's is at least its
	// kin's.
	int poisonResistPercent = 0;
	std::optional<SpitRules> spit; // it spits venom from afar
	bool charges = false;		   // it charges along its row (Charge)
	// A boss's common kin (the Anubis boss: the Anubis guard). A boss has no weakness (WEAK) and resists every damage
	// type at least as well as its kin (docs/plan/solved/boss-resistances.md).
	int kin = 0;
	BossRules boss{};	 // summons minions, its death opens the boss gates (BOSS_LOCK); at most one per level
	GenPick generated{}; // levelgen's random monsters

	[[nodiscard]] constexpr bool isBoss() const { return boss.minion != 0; }
};

// nullptr for an unknown type.
[[nodiscard]] const MonsterKind* monsterKind(int type);
[[nodiscard]] inline bool isBossMonster(int type) {
	const MonsterKind* kind = monsterKind(type);
	return kind != nullptr && kind->isBoss();
}

// Its bite or sting poisons the player. The checker wants an antidote in reach on a level with one
// (docs/plan/solved/poison-and-antidote.md).
[[nodiscard]] inline bool isPoisoner(int type) {
	const MonsterKind* kind = monsterKind(type);
	return kind != nullptr && kind->poison.has_value();
}

// How the journal writes a poison resistance down: "normal", "resists", "tough", "immune" (as resistanceWord).
[[nodiscard]] constexpr const char* poisonResistanceWord(int percent) {
	return percent <= 0 ? "normal" : percent <= 50 ? "resists" : percent < 100 ? "tough" : "immune";
}

constexpr char UNKNOWN_MONSTER_GLYPH = 'm';
constexpr float UNKNOWN_MONSTER_THREAT = 2.f;

#endif
