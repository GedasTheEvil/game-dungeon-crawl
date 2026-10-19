#ifndef MONSTER_KINDS_H
#define MONSTER_KINDS_H

// One row per monster type, everything about it but the loaded model: names, glyph, the checker's threat, stats,
// model and texture paths, resistances, how it moves, poisons, spits and summons. The game loads the model of each
// row into a MonsterType (entities/monster.h); levelgen picks from the rows.

#include "../core/gameplay_config.h"
#include "curses.h"
#include "damage.h"
#include "level.h"
#include "poison.h"
#include "rgb.h"
#include <optional>

// How a monster gets around. Only flyers cross pits; walkers stop at their edge, and at traps unless reckless
// (Courage).
enum class Locomotion : unsigned char {
	Stationary, // rooted to its spawn tile (plant), attacks when the player is next to it
	Ambush, // rooted like Stationary, idle and still (a treasure chest) until the player comes near, see Lurk;
			// killed, it leaves a real treasure chest on its tile
	Walk, // follows the player along its row
	Entombed, // walks, but lies in its coffin (idle) until the player comes near or hits it, then climbs out (Rise)
			  // before it acts
	WalkJump, // walks, leaps over pits and traps (giant rat, see Leap)
	Fly, // see Flight
	Submerged, // walks, but lies idle in the water until the player comes near or hits it (the crocodile)
	// Walks, but lies coiled (idle) until the player comes near or hits it, then rears up (Rise) before it acts (the
	// cobra).
	Coiled,
	Burrow, // walks, and now and then dives into the floor and comes up elsewhere on its row (Apep, see Burrow)
	// Rooted in half water like Stationary, a flower floating on the surface (idle) until the player comes near, then
	// rears up (Rise) and bites where it stands (the blue lotus, plan water-lotus).
	Floating,
	// Walks, but stands still among the decor figures on its cell, as one of them (idle), until the player comes near
	// or hits it, then steps off its place (Rise) before it acts (the living ushabti, plan living-ushabti).
	Figurine,
};

// How a walker moves through half water (crocodiles-and-flooded-cells). Rooted monsters and flyers do not wade.
enum class Wading : unsigned char {
	Slowed, // slower, like the player (MonsterKind::waterSpeed, WADE_SPEED_FACTOR unless set)
	Unaffected, // full speed
	Swimmer, // faster (waterSpeed), floating with its back at the surface (swimLift)
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
// of a coffin by the boss (entombed minions, Monster::Rising), hatch out of a living nest (BossRules::nest;
// emerging like DigOut; none left, no summons), or step out of the wall the player faces, like statues out of the
// stone (Summon::Wall, plan mummy-boss).
enum class Summon : unsigned char { DigOut, Drop, Coffin, Hatch, Wall };

// A boss summons minions around itself while it lives (Dungeon::updateBoss).
struct BossRules {
	int minion = 0; // MonsterTypeId of its minions; 0: not a boss
	int minAlive = 0; // minions around it when it appears
	int maxAlive = 0; // no summon while this many are alive
	int summonMs = 0; // between summons
	int summonCap = 0; // summons per fight, after the first minAlive
	int lifeStealPct = 0; // heals this share of the damage it deals
	Summon summon = Summon::DigOut;
	int nest = 0; // Summon::Hatch: the MonsterTypeId its minions hatch out of
	// Summon::Hatch, Summon::Wall: a smaller kind every other summon (the scorpion queen's scorpions, the Anubis's
	// ushabti); 0: none
	int smallMinion = 0;
};

constexpr Rgb VENOM_GREEN = {0.45f, 0.75f, 0.15f};
constexpr Rgb BOLT_FIRE = {1.f, 0.55f, 0.15f}; // the Anubis's fire bolt, its glowing core
constexpr Rgb BOLT_PURPLE = {0.7f, 0.2f, 0.95f}; // the mummy priest's curse

// A ranged attack: from afar along its row, it stops and shoots at the player (Dungeon::MonsterShot): the cobras'
// spat venom, the Anubis's bolt of light, the mummy priest's spoken curse. The release and its height come from the
// model's ranged clip, measured by its script's --measure (tools/blender/models/cobra.py, anubis.py).
struct RangedRules {
	int damage = 0; // on a hit, of the type's attack mix
	std::optional<PoisonTier> poison; // a hit poisons; nullopt: no poison (the Anubis's bolt)
	float range = 0.f; // tiles between the boxes, at most; nearer than MONSTER_BITE_REACH it bites
	int cooldownMs = 0; // from one shot to the next
	float release = 0.5f; // of the ranged clip: the shot leaves
	float releaseY = 0.5f; // of the reference clip's height: the mouth or sceptre head at the release
	Rgb colour = VENOM_GREEN;
	float releaseAhead = 1.f; // of the half width, ahead of the centre: 1 the hitbox front (HeadX)
	// What a hit deals; nullopt: the type's attackMix (the cobras). With fire it sets the player burning for burnMs
	// and the shot is drawn as a fireball (plan anubis-fire-bolt).
	std::optional<DamageMix> mix;
	int burnMs = BURN_MS;

	[[nodiscard]] constexpr bool burns() const { return mix.has_value() && (*mix)[FIRE] > 0; }
};

// Where levelgen puts it: levels of difficulty minDifficulty .. maxDifficulty, picked by weight. Weight 0: never.
// Thorns and sap: a melee hit on the monster may poison the player; arrows and thrown weapons do not.
struct TouchPoison {
	PoisonTier tier = PoisonTier::Weak;
	int chancePercent = 0;
};

// A charge along the row (Charge): from minRange..maxRange tiles away it winds up, then rushes at speed (tiles per
// second). Sobek's are the defaults (CHARGE_MIN, CHARGE_MAX, CHARGE_SPEED).
struct ChargeRules {
	float minRange = CHARGE_MIN;
	float maxRange = CHARGE_MAX;
	float speed = CHARGE_SPEED;
};

// A bite that grabs and holds the player (Grab, plans crocodile-hold-bite and giant-scorpion-grab): a share of its
// bites turns into a tell (GRAB_TELL_MS, jaws or claws wide), then a lunge; in reach it grabs, else it misses. While
// held the player cannot walk, jump, sprint, drink or open a screen. The crocodile's death roll ticks; the scorpion's
// hold ends in a sure sting, and the player's hits on it shorten the hold (break free: no sting).
struct HoldRules {
	int chancePercent = 0; // of its bites, rolled on the gameplay stream
	bool inWater = false; // only while the player stands in half water (Dungeon::PlayerWading)
	int grabDamagePct = 100; // the grab's hit, of its damage
	std::optional<DamageMix> grabMix; // nullopt: its attackMix
	int holdMs = 0; // from the grab to the end of the hold
	int tickMs = 0; // a tick every tickMs of the hold, up to its end; 0: none
	int tickDamagePct = 0; // a tick's hit, of its damage, armour counts
	int breakMs = 0; // each of the player's hits on it ends the hold this much sooner; 0: no attack while held
	bool sting = false; // at the hold's end a sure hit: its damage and its poison
	// Frames of its attack clip shown frozen (tools/blender/models/*.py ATTACK): the tell, the hold.
	float tellFrame = 0.f;
	float holdFrame = 0.f;
};

struct GenPick {
	int minDifficulty = 0;
	int maxDifficulty = 0;
	int weight = 0;
};

constexpr Rgb RED_BLOOD = {0.7f, 0.1f, 0.1f};

struct MonsterKind {
	MonsterTypeId id{};
	char glyph = 'm'; // levelcheck --map and the ASCII level sources
	const char* name = ""; // the game's and the journal's ("Man-eater plant")
	const char* label = ""; // the editor's list ("giant rat")
	const char* description = ""; // the editor's hint
	const char* note = ""; // the journal's description, in the archaeologist's words, written after the first kill
	float threat = 0.f; // one of them in the checker's difficulty score

	// Look: under media/models/ and media/sounds/, under media/textures/ (a giant kin is the same model, bigger and
	// darker).
	const char* model = "";
	const char* texture = "";
	float scale = 1.f;
	float rotA = 180.f; // model yaw facing the camera
	Rgb blood = RED_BLOOD;

	float speed = 1.f;
	int maxHealth = 20;
	int damage = 1;
	int attackMs = 800; // between bites
	int xp = 0; // gained for the kill
	DamageMix attackMix{}; // what its bite deals; shared by a model's group
	Resistances resist = NO_RESISTANCES; // how it takes each type of a weapon's damage

	Locomotion locomotion = Locomotion::Walk;
	Courage courage = Courage::Coward;
	int trapDamagePct = 100; // share of a trap's damage it takes (traps ignore armour); 0: immune
	// Presses a dart trap's plate at DART_PLATE_WEIGHT and up (plan poison-dart-trap): 0 flyers, 1 small ones (rats,
	// scarabs), 2-3 large ones, more for bosses. Later traps may want more.
	int weight = 2;
	Wading wading = Wading::Slowed;
	float waterSpeed = WADE_SPEED_FACTOR; // its speed in half water, times its speed on land
	std::optional<PoisonTier> poison; // its bite or sting poisons the player
	int poisonChancePercent = 100; // the share of its bites that poison
	std::optional<TouchPoison> touch; // a melee hit on it poisons the player (plan plant-poison)
	// Chance that a poisoning does not take (plan monster-poison); 100: immune. A boss's is at least its
	// kin's.
	int poisonResistPercent = 0;
	std::optional<RangedRules> ranged; // a ranged attack from afar: it spits venom or shoots a bolt
	int staminaDrainPct = 0; // a bite takes this share of the player's max stamina (the blue lotus's sedative)
	// Curses its hits lay (plan curses): its bites and blows (a charge too), its spit or bolt. The strongest first; a
	// boss's chances are higher than its kin's.
	CurseEntries meleeCurses{};
	CurseEntries rangedCurses{};
	// Time its rise clip takes (Rising), stepped to FRAME_STEP_MS; 0: the clip at the usual speed. Short, so the
	// player jumping on it does not kill it before it can hit once.
	int riseMs = 0;
	// AdvancePlayback's speed of its walk clip (plan anubis-walk-clip-speed), so the legs keep up with a fast walker.
	int walkClipSpeed = 35; // ModelInfo::CLIP_SPEED (model_info.h includes this header; a unit test checks it)
	std::optional<ChargeRules> charge; // it charges along its row (Charge)
	std::optional<HoldRules> hold; // some of its bites grab and hold the player (Grab)
	bool shunsLight = false; // it does not step into a lit cell (Dungeon::Lit, plan camel-spider)
	// Follows the player up and down ladders once it has seen them (plan monster-climbers).
	bool climbs = false;
	// Its model's climb clip (tools/blender/models/*.py prints both), as shares of the reference clip's frame 0
	// height: how high it climbs in one cycle of the clip, and how far in front of its centre it grips the ladder.
	// climbRise 0: no climb clip, the walk clip stands in on the clock.
	float climbRise = 0.f;
	float climbGrip = 0.f;
	// A boss's common kin (the Anubis boss: the Anubis guard). A boss has no weakness (WEAK) and resists every damage
	// type at least as well as its kin (plan boss-resistances).
	int kin = 0;
	BossRules boss{}; // summons minions, its death opens the boss gates (BOSS_LOCK); at most one per level
	GenPick generated{}; // levelgen's random monsters

	[[nodiscard]] constexpr bool isBoss() const { return boss.minion != 0; }
};

// nullptr for an unknown type.
[[nodiscard]] const MonsterKind* monsterKind(int type);
[[nodiscard]] inline bool isBossMonster(int type) {
	const MonsterKind* kind = monsterKind(type);
	return kind != nullptr && kind->isBoss();
}

// Its bite or sting poisons the player. The checker wants an antidote in reach on a level with one (plan
// poison-and-antidote).
[[nodiscard]] inline bool isPoisoner(int type) {
	const MonsterKind* kind = monsterKind(type);
	return kind != nullptr && (kind->poison.has_value() || kind->touch.has_value());
}

// How the journal writes a poison resistance down: "normal", "resists", "tough", "immune" (as resistanceWord).
[[nodiscard]] constexpr const char* poisonResistanceWord(int percent) {
	return percent <= 0 ? "normal" : percent <= 50 ? "resists" : percent < 100 ? "tough" : "immune";
}

constexpr char UNKNOWN_MONSTER_GLYPH = 'm';
constexpr float UNKNOWN_MONSTER_THREAT = 2.f;

#endif
