#include "monster_kinds.h"
#include <array>

namespace {
constexpr Rgb SCARAB_BLOOD = {0.6f, 0.1f, 0.8f};
constexpr Rgb SCORPION_BLOOD = {0.45f, 0.62f, 0.55f};

// What each model group's bite or blow deals (docs/plan/solved/monster-attack-damage-types.md), read from what the
// model attacks with. The members of a group (bigger or darker) deal the same kinds, only more.
constexpr DamageMix WORM_MIX = {60, 40, 0};		   // grinding maw
constexpr DamageMix SCARAB_MIX = {40, 60, 0};	   // rams, mandibles
constexpr DamageMix PLANT_MIX = {0, 40, 60};	   // bite, thorny vines
constexpr DamageMix RAT_MIX = {0, 30, 70};		   // teeth
constexpr DamageMix BAT_MIX = {0, 20, 80};		   // fangs, claws
constexpr DamageMix MUMMY_MIX = {100, 0, 0};	   // fists
constexpr DamageMix ANUBIS_MIX = {80, 0, 20};	   // was-sceptre, its forked foot
constexpr DamageMix CROCODILE_MIX = {50, 0, 50};   // crushing jaws
constexpr DamageMix SCORPION_MIX = {0, 30, 70};	   // claws, sting
constexpr DamageMix MIMIC_MIX = {40, 0, 60};	   // lid slam, teeth
constexpr DamageMix COBRA_MIX = {0, 0, 100};	   // fangs
constexpr DamageMix EGG_CLUSTER_MIX = {100, 0, 0}; // never bites

// How each group takes blunt, slash and pierce damage (docs/plan/solved/damage-types-and-resistances.md). Each weapon
// is the best against some: the club against bats, mimics and the Anubis guard, the sword against worms, plants and
// mummies, the spear (and the bow) against scarabs. A boss has no weak spot (bossResist).
constexpr Resistances WORM_RESIST = {RESISTS, WEAK, NORMAL};   // soft: a blow squashes, a blade cuts
constexpr Resistances SCARAB_RESIST = {NORMAL, RESISTS, WEAK}; // the shell turns a blade, a point goes between plates
constexpr Resistances PLANT_RESIST = {TOUGH, WEAK, TOUGH};	   // stems: only a blade cuts; points and blows go astray
constexpr Resistances BAT_RESIST = {WEAK, RESISTS, TOUGH};	   // swat it; an arrow goes through the wing
constexpr Resistances MIMIC_RESIST = {WEAK, RESISTS, TOUGH};   // wood: crack it; a point only sticks in it
constexpr Resistances ANUBIS_RESIST = {WEAK, RESISTS, NORMAL}; // bronze armour dents, a blade glances off it
constexpr Resistances MUMMY_RESIST = {RESISTS, WEAK, TOUGH};   // dry linen tears; nothing inside to stab
constexpr Resistances CROCODILE_RESIST = {NORMAL, RESISTS, NORMAL}; // the scutes turn a blade
constexpr Resistances SCORPION_RESIST = {WEAK, NORMAL, RESISTS};	// a blow cracks the thin shell; a point glances off
constexpr Resistances COBRA_RESIST = {RESISTS, WEAK, NORMAL}; // the coils give under a blow; a blade cuts the body
constexpr Resistances EGG_CLUSTER_RESIST = {NORMAL, WEAK, RESISTS}; // a blade slits the leathery eggs

// Chance a poisoning does not take (docs/plan/monster-poison.md). The rest have none, like the player without an
// amulet. Immune: no blood to carry it (the dead, bronze, a stem, wood, a nest of eggs).
constexpr int POISON_IMMUNE = 100;
constexpr int POISONER_RESIST = 50;		 // scorpions and cobras: half used to venom
constexpr int POISONER_BOSS_RESIST = 75; // the scorpion queen, Apep: more than their kin

// A boss has no weakness: where its kin is weak, it takes the damage normally.
constexpr Resistances bossResist(Resistances kin) {
	for (int& rate : kin)
		rate = rate > NORMAL ? NORMAL : rate;
	return kin;
}

// By MonsterTypeId, from 1. Small ones are quick and bite often but barely hurt; big ones are slow, hit hard and take
// long to kill. Flyers roost on the ceiling and swoop through the player (Monster::Fly). The boss summons are starting
// values, to tune from playthroughs (docs/plan/solved/boss-rooms.md).
constexpr std::array<MonsterKind, MONSTER_TYPE_MAX> KINDS = {{
	{.id = MonsterScarab,
	 .glyph = 's',
	 .name = "Scarab",
	 .label = "scarab",
	 .description = "Scarab",
	 .note = "A beetle the size of my fist. Its bite stings.",
	 .threat = 0.8f,
	 .model = "monsters/scarab",
	 .texture = "monsters/scarab",
	 .scale = 7,
	 .blood = SCARAB_BLOOD,
	 .speed = 4,
	 .maxHealth = 10,
	 .damage = 2,
	 .attackMs = 600,
	 .xp = 300,
	 .attackMix = SCARAB_MIX,
	 .resist = SCARAB_RESIST,
	 .weight = 1,
	 .generated = {1, 5, 4}},
	{.id = MonsterWorm,
	 .glyph = 'w',
	 .name = "Worm",
	 .label = "worm",
	 .description = "Worm",
	 .note = "Slow as sand, but its jaws close like a vice.",
	 .threat = 2.f,
	 .model = "monsters/worm",
	 .texture = "monsters/worm",
	 .scale = 18,
	 .rotA = 0,
	 .speed = 1,
	 .maxHealth = 30,
	 .damage = 9,
	 .attackMs = 1000,
	 .xp = 1200,
	 .attackMix = WORM_MIX,
	 .resist = WORM_RESIST,
	 .generated = {3, 7, 2}},
	{.id = MonsterPlant,
	 .glyph = 'p',
	 .name = "Man-eater plant",
	 .label = "plant",
	 .description = "Plant",
	 .note = "Rooted to the spot. Never stand next to it.",
	 .threat = 1.5f, // does not move
	 .model = "monsters/plant",
	 .texture = "monsters/plant",
	 .scale = 12,
	 .rotA = 0,
	 .blood = {0.1f, 0.4f, 0.1f},
	 .speed = 0,
	 .maxHealth = 30,
	 .damage = 5,
	 .attackMs = 800,
	 .xp = 1000,
	 .attackMix = PLANT_MIX,
	 .resist = PLANT_RESIST,
	 .locomotion = Locomotion::Stationary,
	 .poisonResistPercent = POISON_IMMUNE,
	 .generated = {3, 10, 2}},
	// Reckless like the Anubis boss; its trap share is between the boss (10%) and the mummy (50%). Strong enough for
	// the late levels (13-30): a level 55 player (about 700 HP, 11 armour) takes about 15 blows.
	{.id = MonsterAnubis,
	 .glyph = 'n',
	 .name = "Anubis",
	 .label = "anubis",
	 .description = "Anubis",
	 .note = "A jackal-headed guard. Traps do not stop him.",
	 .threat = 8.f,
	 .model = "monsters/anubis",
	 .texture = "monsters/anubis",
	 .scale = 19,
	 .speed = 3,
	 .maxHealth = 600,
	 .damage = 55,
	 .attackMs = 1100,
	 .xp = 10000,
	 .attackMix = ANUBIS_MIX,
	 .resist = ANUBIS_RESIST,
	 .courage = Courage::Reckless,
	 .trapDamagePct = 25,
	 .weight = 3,
	 .wading = Wading::Unaffected,
	 .waterSpeed = 1.f,
	 .poisonResistPercent = POISON_IMMUNE,
	 .generated = {9, 10, 1}},
	{.id = MonsterRat,
	 .glyph = 't',
	 .name = "Rat",
	 .label = "rat",
	 .description = "Rat",
	 .note = "Quick, filthy and always hungry.",
	 .threat = 0.7f, // fast, but barely hurts
	 .model = "monsters/rat",
	 .texture = "monsters/rat",
	 .scale = 13,
	 .speed = 9,
	 .maxHealth = 12,
	 .damage = 2,
	 .attackMs = 400,
	 .xp = 300,
	 .attackMix = RAT_MIX,
	 .weight = 1,
	 .wading = Wading::Unaffected,
	 .waterSpeed = 1.f,
	 .generated = {1, 5, 6}},
	// Faster than the player's walk (~1.25 tiles/s): the bow alone does not keep it off
	// (docs/plan/solved/giant-rat-speed.md).
	{.id = MonsterGiantRat,
	 .glyph = 'T',
	 .name = "Giant rat",
	 .label = "giant rat",
	 .description = "Giant rat",
	 .note = "A rat as big as a dog. Jumps at me over the pits.",
	 .threat = 3.f,
	 .model = "monsters/rat",
	 .texture = "monsters/rat_giant",
	 .scale = 42,
	 .speed = 24,
	 .maxHealth = 60,
	 .damage = 8,
	 .attackMs = 700,
	 .xp = 2000,
	 .attackMix = RAT_MIX,
	 .locomotion = Locomotion::WalkJump,
	 .wading = Wading::Unaffected,
	 .waterSpeed = 1.f,
	 .generated = {4, 10, 3}},
	{.id = MonsterBat,
	 .glyph = 'f',
	 .name = "Bat",
	 .label = "bat",
	 .description = "Bat",
	 .note = "Flits down out of the dark. Hard to hit in the air.",
	 .threat = 1.2f, // weak, but only hit with good timing
	 .model = "monsters/bat",
	 .texture = "monsters/bat",
	 .scale = 18,
	 .speed = 5,
	 .maxHealth = 8,
	 .damage = 3,
	 .attackMs = 800,
	 .xp = 400,
	 .attackMix = BAT_MIX,
	 .resist = BAT_RESIST,
	 .locomotion = Locomotion::Fly,
	 .weight = 0,
	 .generated = {2, 6, 3}},
	{.id = MonsterGiantBat,
	 .glyph = 'F',
	 .name = "Giant bat",
	 .label = "giant bat",
	 .description = "Giant bat",
	 .note = "Its wings are wider than my arms.",
	 .threat = 3.5f,
	 .model = "monsters/bat",
	 .texture = "monsters/bat_giant",
	 .scale = 30,
	 .blood = {0.45f, 0.05f, 0.05f},
	 .speed = 4,
	 .maxHealth = 40,
	 .damage = 10,
	 .attackMs = 800,
	 .xp = 1800,
	 .attackMix = BAT_MIX,
	 .resist = BAT_RESIST,
	 .locomotion = Locomotion::Fly,
	 .weight = 0,
	 .generated = {6, 10, 2}},
	// Scale and yaw of the treasure chest item: idle, it looks just like one.
	{.id = MonsterMimic,
	 .glyph = 'M',
	 .name = "Mimic",
	 .label = "mimic",
	 .description = "Mimic, a treasure chest until the player comes near",
	 .note = "Not every chest in this tomb holds treasure.",
	 .threat = 2.f, // hits hard, but does not move
	 .model = "monsters/mimic",
	 .texture = "monsters/mimic",
	 .scale = 8,
	 .rotA = 0,
	 .blood = {0.5f, 0.05f, 0.1f},
	 .speed = 0,
	 .maxHealth = 40,
	 .damage = 10,
	 .attackMs = 800,
	 .xp = 1500,
	 .attackMix = MIMIC_MIX,
	 .resist = MIMIC_RESIST,
	 .locomotion = Locomotion::Ambush,
	 .poisonResistPercent = POISON_IMMUNE},
	{.id = MonsterGiantScarab,
	 .glyph = 'k',
	 .name = "Giant scarab",
	 .label = "giant scarab",
	 .description = "Giant scarab, leaps over pits and traps",
	 .note = "A shell like a shield, and it jumps.",
	 .threat = 4.f, // jumps like a giant rat, hits harder
	 .model = "monsters/scarab",
	 .texture = "monsters/scarab_giant",
	 .scale = 24,
	 .blood = {0.4f, 0.05f, 0.55f},
	 .speed = 2,
	 .maxHealth = 90,
	 .damage = 12,
	 .attackMs = 1000,
	 .xp = 2500,
	 .attackMix = SCARAB_MIX,
	 .resist = SCARAB_RESIST,
	 .locomotion = Locomotion::WalkJump,
	 .generated = {5, 10, 3}},
	// Bigger, faster and harder than the giant scarab. A full clear of levels 1-4 makes the player level 8 (134 HP):
	// 3-4 bites kill them.
	{.id = MonsterBossScarab,
	 .glyph = 'K',
	 .name = "Boss scarab",
	 .label = "boss scarab",
	 .description = "Boss scarab, summons scarabs; its death opens the boss gates. One boss per level",
	 .note = "The mother of the scarabs. Her brood comes when she calls.",
	 .threat = 10.f, // a little weaker than an Anubis, plus its scarabs
	 .model = "monsters/scarab",
	 .texture = "monsters/scarab_boss",
	 .scale = 34,
	 .blood = {0.1f, 0.2f, 0.75f},
	 .speed = 3,
	 .maxHealth = 320,
	 .damage = 40,
	 .attackMs = 900,
	 .xp = 6000,
	 .attackMix = SCARAB_MIX,
	 .resist = bossResist(SCARAB_RESIST),
	 .locomotion = Locomotion::WalkJump,
	 .weight = 4,
	 .kin = MonsterScarab,
	 .boss = {MonsterScarab, 3, 5, 1500, 12, 0, Summon::DigOut}},
	// A giant bat grown fat on blood. The player comes to lvl10 at about level 21 (290 HP): 3-4 bites kill them.
	{.id = MonsterVampireBat,
	 .glyph = 'V',
	 .name = "Vampire bat",
	 .label = "vampire bat",
	 .description = "Vampire bat, summons bats and heals by part of the damage it deals; its death opens the boss "
					"gates. One boss per level",
	 .note = "Grown fat on blood. Every bite makes it stronger.",
	 .threat = 12.f, // a giant bat that hits like a boss and heals, plus its bats
	 .model = "monsters/bat",
	 .texture = "monsters/bat_vampire",
	 .scale = 42,
	 .blood = {0.5f, 0.02f, 0.08f},
	 .speed = 4,
	 .maxHealth = 400,
	 .damage = 80,
	 .attackMs = 800,
	 .xp = 12000,
	 .attackMix = BAT_MIX,
	 .resist = bossResist(BAT_RESIST),
	 .locomotion = Locomotion::Fly,
	 .weight = 0,
	 .kin = MonsterBat,
	 .boss = {MonsterBat, 2, 4, 2000, 8, 30, Summon::Drop}},
	// Fast for its bulk, hits hard and slowly. Levels 11 on, the Anubis boss's minion (docs/plan/solved/boss-rooms.md).
	{.id = MonsterMummy,
	 .glyph = 'u',
	 .name = "Mummy",
	 .label = "mummy",
	 .description = "Mummy, lies in its coffin until the player comes near",
	 .note = "Wrapped and patient. It waited three thousand years for me.",
	 .threat = 5.f, // walks fast for its size and hits hard, but gives the player time to see it climb out
	 .model = "monsters/mummy",
	 .texture = "monsters/mummy",
	 .scale = 18,
	 .blood = {0.35f, 0.25f, 0.12f},
	 .speed = 2.5f,
	 .maxHealth = 150,
	 .damage = 20,
	 .attackMs = 1600,
	 .xp = 2500,
	 .attackMix = MUMMY_MIX,
	 .resist = MUMMY_RESIST,
	 .locomotion = Locomotion::Entombed,
	 .courage = Courage::Reckless, // a crushing rock (500) can kill it
	 .trapDamagePct = 50,
	 .poisonResistPercent = POISON_IMMUNE,
	 .generated = {7, 10, 2}},
	// The finale's guardian, a head taller than an Anubis, the strongest boss. The player comes to lvl30 at about
	// level 55 (about 700 HP, 11 armour): 6 blows kill them. Reckless: the player cannot shake him off behind a row of
	// traps.
	{.id = MonsterAnubisBoss,
	 .glyph = 'N',
	 .name = "Anubis boss",
	 .label = "anubis boss",
	 .description = "Anubis boss, mummies climb out of the coffins round it; its death opens the boss gates. One boss "
					"per level",
	 .note = "The guardian of the last tomb. The dead rise at his word.",
	 .threat = 15.f, // the finale: four hits kill a level 30 player, plus its mummies
	 .model = "monsters/anubis",
	 .texture = "monsters/anubis_boss",
	 .scale = 26,
	 .speed = 4.5f,
	 .maxHealth = 2400,
	 .damage = 140,
	 .attackMs = 1300,
	 .xp = 30000,
	 .attackMix = ANUBIS_MIX,
	 .resist = bossResist(ANUBIS_RESIST),
	 .courage = Courage::Reckless,
	 .trapDamagePct = 10,
	 .weight = 4,
	 .wading = Wading::Unaffected,
	 .waterSpeed = 1.f,
	 .poisonResistPercent = POISON_IMMUNE,
	 .kin = MonsterAnubis,
	 .boss = {MonsterMummy, 2, 4, 2000, 10, 0, Summon::Coffin}},
	// HP between the giant rat and the mummy, bites harder than both. Slow on land (slower than a rat: the player
	// outwalks it), in the water faster than the player walks on land. Levels 7-9
	// (docs/plan/solved/crocodiles-and-flooded-cells.md). A long, low body: 1.5 tiles nose to tail.
	{.id = MonsterCrocodile,
	 .glyph = 'C',
	 .name = "Crocodile",
	 .label = "crocodile",
	 .description = "Crocodile, lies under the water until the player comes near, swims fast",
	 .note = "Sobek's own. In the water I cannot outrun it.",
	 .threat = 4.5f, // between the giant rat and the mummy; in the water it outswims the player
	 .model = "monsters/crocodile",
	 .texture = "monsters/crocodile",
	 .scale = 60,
	 .speed = 7,
	 .maxHealth = 110,
	 .damage = 26,
	 .attackMs = 1100,
	 .xp = 2400,
	 .attackMix = CROCODILE_MIX,
	 .resist = CROCODILE_RESIST,
	 .locomotion = Locomotion::Submerged,
	 .weight = 3,
	 .wading = Wading::Swimmer,
	 .waterSpeed = 2.5f},
	// Small and quick like the rat, a slower sting that poisons; levels from the scorpion queen's
	// (docs/plan/solved/poison-and-antidote.md).
	{.id = MonsterScorpion,
	 .glyph = 'j',
	 .name = "Scorpion",
	 .label = "scorpion",
	 .description = "Scorpion, its sting poisons (weak)",
	 .note = "Pale as straw, quick on its eight legs. The sting burns for a long while.",
	 .threat = 1.f, // a little above the rat and the scarab: the poison burns on after the fight
	 .model = "monsters/scorpion",
	 .texture = "monsters/scorpion",
	 .scale = 15,
	 .blood = SCORPION_BLOOD,
	 .speed = 10,
	 .maxHealth = 14,
	 .damage = 3,
	 .attackMs = 900,
	 .xp = 450,
	 .attackMix = SCORPION_MIX,
	 .resist = SCORPION_RESIST,
	 .weight = 1,
	 .poison = PoisonTier::Weak,
	 .poisonResistPercent = POISONER_RESIST},
	// Between the giant rat and the crocodile, a poisoned bite and a venom spit from afar; lies coiled until the player
	// comes near (docs/plan/cobra.md).
	{.id = MonsterCobra,
	 .glyph = 'c',
	 .name = "Cobra",
	 .label = "cobra",
	 .description = "Cobra, lies coiled until the player comes near; its bite and spit poison (medium)",
	 .note = "The uraeus of the crowns, alive. It spits before it bites, and both burn.",
	 .threat = 3.f, // a giant rat's, the poison on top, and it spits from afar
	 .model = "monsters/cobra",
	 .texture = "monsters/cobra",
	 .scale = 24,
	 .speed = 6,
	 .maxHealth = 35,
	 .damage = 6,
	 .attackMs = 1100,
	 .xp = 1200,
	 .attackMix = COBRA_MIX,
	 .resist = COBRA_RESIST,
	 .locomotion = Locomotion::Coiled,
	 .weight = 1,
	 .wading = Wading::Swimmer,
	 .waterSpeed = 1.25f,
	 .poison = PoisonTier::Medium,
	 .poisonResistPercent = POISONER_RESIST,
	 .spit = SpitRules{2, PoisonTier::Medium, 2.5f, 3500, 0.41f, 0.87f}}, // release: frame 7 of 18
	// The cobra's giant kin, levels after Apep (docs/plan/giant-cobra.md).
	{.id = MonsterGiantCobra,
	 .glyph = 'Q',
	 .name = "Giant cobra",
	 .label = "giant cobra",
	 .description = "Giant cobra, lies coiled until the player comes near; its bite and spit poison (medium)",
	 .note = "Black as the night of Apep, long as a boat. It spits farther than I can jump.",
	 .threat = 5.f, // between the giant scarab and the crocodile, the poison and the spit on top
	 .model = "monsters/cobra",
	 .texture = "monsters/cobra_giant",
	 .scale = 36,
	 .speed = 8,
	 .maxHealth = 160,
	 .damage = 26,
	 .attackMs = 1300,
	 .xp = 2600,
	 .attackMix = COBRA_MIX,
	 .resist = COBRA_RESIST,
	 .locomotion = Locomotion::Coiled,
	 .wading = Wading::Swimmer,
	 .waterSpeed = 1.25f,
	 .poison = PoisonTier::Medium,
	 .poisonResistPercent = POISONER_RESIST,
	 .spit = SpitRules{5, PoisonTier::Medium, 3.f, 3000, 0.41f, 0.87f}},
	// The scorpion's giant kin and the scorpion queen's minion: medium poison (docs/plan/scorpion-queen-boss.md).
	{.id = MonsterGiantScorpion,
	 .glyph = 'J',
	 .name = "Giant scorpion",
	 .label = "giant scorpion",
	 .description = "Giant scorpion, its sting poisons (medium)",
	 .note = "Dark as old blood and as big as a dog. Its sting does not wear off quickly.",
	 .threat = 3.5f, // a giant rat's, the poison on top
	 .model = "monsters/scorpion",
	 .texture = "monsters/scorpion_giant",
	 .scale = 26,
	 .blood = SCORPION_BLOOD,
	 .speed = 9,
	 .maxHealth = 70,
	 .damage = 12,
	 .attackMs = 1000,
	 .xp = 2000,
	 .attackMix = SCORPION_MIX,
	 .resist = SCORPION_RESIST,
	 .poison = PoisonTier::Medium,
	 .poisonResistPercent = POISONER_RESIST},
	// Rooted and harmless: the scorpion queen's brood hatches from it (Summon::Hatch). Dies to a few blows.
	{.id = MonsterEggCluster,
	 .glyph = 'e',
	 .name = "Egg cluster",
	 .label = "egg cluster",
	 .description = "Egg cluster, harmless; the scorpion queen's brood hatches from it",
	 .note = "Leathery eggs glued in resin. Something moves inside. Smash them before they hatch.",
	 .threat = 0.5f, // none of its own, but the queen's brood hatches from it
	 .model = "monsters/egg_cluster",
	 .texture = "monsters/egg_cluster",
	 .scale = 16,
	 .rotA = 0,
	 .blood = {0.85f, 0.75f, 0.35f},
	 .speed = 0,
	 .maxHealth = 60,
	 .damage = 0,
	 .attackMs = 1000,
	 .xp = 300,
	 .attackMix = EGG_CLUSTER_MIX,
	 .resist = EGG_CLUSTER_RESIST,
	 .locomotion = Locomotion::Stationary,
	 .poisonResistPercent = POISON_IMMUNE},
	// The lvl15 boss on the scorpion model, the size of a cart (docs/plan/scorpion-queen-boss.md).
	{.id = MonsterScorpionQueen,
	 .glyph = 'U',
	 .name = "Scorpion queen",
	 .label = "scorpion queen",
	 .description = "Scorpion queen, giant scorpions hatch from the egg clusters in her room; her sting poisons "
					"(strong); her death opens the boss gates. One boss per level",
	 .note = "Serket herself, or her daughter. Her sting burns like fire, and her eggs are everywhere.",
	 .threat = 13.f, // between the vampire bat and the anubis boss, plus her brood
	 .model = "monsters/scorpion",
	 .texture = "monsters/scorpion_queen",
	 .scale = 48,
	 .blood = {0.5f, 0.7f, 0.6f},
	 .speed = 5,
	 .maxHealth = 700,
	 .damage = 45,
	 .attackMs = 1100,
	 .xp = 15000,
	 .attackMix = SCORPION_MIX,
	 .resist = bossResist(SCORPION_RESIST),
	 .courage = Courage::Reckless,
	 .trapDamagePct = 10,
	 .weight = 4,
	 .poison = PoisonTier::Strong,
	 .poisonResistPercent = POISONER_BOSS_RESIST,
	 .kin = MonsterScorpion,
	 .boss = {MonsterGiantScorpion, 2, 4, 3000, 10, 0, Summon::Hatch, MonsterEggCluster, MonsterScorpion}},
	// The lvl20 boss on the cobra model, long as the hall (docs/plan/apep-serpent-boss.md).
	{.id = MonsterApep,
	 .glyph = 'P',
	 .name = "Apep",
	 .label = "apep",
	 .description = "Apep, dives into the floor and comes up behind the player; cobras dig out round him; his death "
					"opens the boss gates. One boss per level",
	 .note = "The serpent of chaos, who swallows the sun each night. The sand is his road.",
	 .threat = 14.f, // hard to pin down, plus his cobras
	 .model = "monsters/cobra",
	 .texture = "monsters/cobra_apep",
	 .scale = 70,
	 .blood = {0.35f, 0.05f, 0.1f},
	 .speed = 6,
	 .maxHealth = 1100,
	 .damage = 60,
	 .attackMs = 1200,
	 .xp = 20000,
	 .attackMix = COBRA_MIX,
	 .resist = bossResist(COBRA_RESIST),
	 .locomotion = Locomotion::Burrow,
	 .courage = Courage::Reckless,
	 .trapDamagePct = 10,
	 .weight = 4,
	 .wading = Wading::Swimmer,
	 .waterSpeed = 1.25f,
	 .poisonResistPercent = POISONER_BOSS_RESIST,
	 .kin = MonsterCobra,
	 .boss = {MonsterCobra, 2, 4, 3000, 10, 0, Summon::DigOut}},
	// The lvl25 boss on the crocodile model (docs/plan/sobek-boss.md).
	{.id = MonsterSobek,
	 .glyph = 'W',
	 .name = "Sobek",
	 .label = "sobek",
	 .description = "Sobek, lies in the water, charges along the row (a wall stuns him); crocodiles come out round "
					"him; his death opens the boss gates. One boss per level",
	 .note = "The lord of the Nile, green as the river. He charges like a flood; let the wall stop him.",
	 .threat = 14.5f, // a charge that hits like a cart, plus his crocodiles
	 .model = "monsters/crocodile",
	 .texture = "monsters/crocodile_sobek",
	 .scale = 100,
	 .speed = 6,
	 .maxHealth = 1600,
	 .damage = 70,
	 .attackMs = 1300,
	 .xp = 25000,
	 .attackMix = CROCODILE_MIX,
	 .resist = bossResist(CROCODILE_RESIST),
	 .locomotion = Locomotion::Submerged,
	 .courage = Courage::Reckless,
	 .trapDamagePct = 10,
	 .weight = 5,
	 .wading = Wading::Swimmer,
	 .waterSpeed = 2.5f,
	 .charges = true,
	 .kin = MonsterCrocodile,
	 .boss = {MonsterCrocodile, 1, 3, 5000, 6, 0, Summon::DigOut}},
}};

constexpr bool validKinds() {
	for (size_t i = 0; i < KINDS.size(); i++) {
		const MonsterKind& kind = KINDS[i];
		if (kind.id != static_cast<int>(i) + 1 || kind.attackMix[0] + kind.attackMix[1] + kind.attackMix[2] != 100)
			return false;
		if (kind.boss.minion < 0 || kind.boss.minion > MONSTER_TYPE_MAX || kind.boss.smallMinion < 0 ||
			kind.boss.smallMinion > MONSTER_TYPE_MAX || (kind.boss.summon == Summon::Hatch) != (kind.boss.nest != 0))
			return false;
		if (kind.isBoss() != (kind.kin != 0) || kind.kin < 0 || kind.kin > MONSTER_TYPE_MAX)
			return false;
		if (kind.poisonResistPercent < 0 || kind.poisonResistPercent > POISON_IMMUNE ||
			(kind.isBoss() && kind.poisonResistPercent < KINDS[static_cast<size_t>(kind.kin - 1)].poisonResistPercent))
			return false;
		for (size_t t = 0; kind.isBoss() && t < kind.resist.size(); t++)
			if (kind.resist[t] > NORMAL || kind.resist[t] > KINDS[static_cast<size_t>(kind.kin - 1)].resist[t])
				return false;
	}
	return true;
}
static_assert(validKinds(), "KINDS: one row per MonsterTypeId in order, attack mixes of 100%, hatchers with a nest, "
							"bosses with a kin, no weakness and no resistance worse than the kin's, poison included");
} // namespace

const MonsterKind* monsterKind(int type) {
	return type >= 1 && type <= MONSTER_TYPE_MAX ? &KINDS[static_cast<size_t>(type - 1)] : nullptr;
}
