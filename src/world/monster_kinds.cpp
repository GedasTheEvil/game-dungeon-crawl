#include "monster_kinds.h"
#include <array>

namespace {
constexpr Rgb SCARAB_BLOOD = {0.6f, 0.1f, 0.8f};
constexpr Rgb SCORPION_BLOOD = {0.45f, 0.62f, 0.55f};

// What each model group's bite or blow deals (plan monster-attack-damage-types), read from what the model attacks with.
// The members of a group (bigger or darker) deal the same kinds, only more.
constexpr DamageMix WORM_MIX = {60, 40, 0}; // grinding maw
constexpr DamageMix SCARAB_MIX = {40, 60, 0}; // rams, mandibles
constexpr DamageMix PLANT_MIX = {0, 40, 60}; // bite, thorny vines
constexpr DamageMix RAT_MIX = {0, 30, 70}; // teeth
constexpr DamageMix BAT_MIX = {0, 20, 80}; // fangs, claws
constexpr DamageMix MUMMY_MIX = {100, 0, 0}; // fists
constexpr DamageMix ANUBIS_MIX = {80, 0, 20}; // was-sceptre, its forked foot
// Its bolt: fire with a stone knock, fire 100 : blunt 40 : pierce 20; the physical share keeps it a threat through the
// fire ward's 80% (plan anubis-fire-bolt).
constexpr DamageMix ANUBIS_BOLT_MIX = {25, 0, 13, 62};
constexpr int ANUBIS_BOSS_BURN_MS = 5000; // its guard's: BURN_MS
constexpr DamageMix CROCODILE_MIX = {50, 0, 50}; // crushing jaws
constexpr DamageMix SCORPION_MIX = {0, 30, 70}; // claws, sting
constexpr DamageMix MIMIC_MIX = {40, 0, 60}; // lid slam, teeth
constexpr DamageMix COBRA_MIX = {0, 0, 100}; // fangs
constexpr DamageMix EGG_CLUSTER_MIX = {100, 0, 0}; // never bites
constexpr DamageMix LOTUS_MIX = {0, 30, 70}; // fangs in the petals
constexpr DamageMix CAMEL_SPIDER_MIX = {0, 80, 20}; // jaws like shears
constexpr DamageMix HIPPO_MIX = {70, 0, 30}; // jaws that crush, the tusks
constexpr DamageMix USHABTI_MIX = {60, 0, 40}; // the hoe: a knock, its blade
constexpr DamageMix BABOON_MIX = {0, 50, 50}; // long fangs that slash and stab
// Its coal from the lake: a hot stone's knock, mostly fire; the knock keeps it a threat through the fire ward.
constexpr DamageMix BABOON_COAL_MIX = {30, 0, 0, 70};
constexpr DamageMix BABI_MIX = {40, 30, 30}; // Babi: a two-handed blow, then the fangs

// How each group takes blunt, slash, pierce and fire damage (plans damage-types-and-resistances, fire-damage). Each
// weapon is the best against some: the club against bats, mimics and the Anubis guard, the sword against worms, plants
// and mummies, the spear (and the bow) against scarabs, fire (a brazier's arrows) against mummies, plants, worms and
// eggs. A boss has no weak spot (bossResist).
// The rule (plan skill-flaming-arrows): whoever resists pierce (an arrow barely scratches it) is weak to fire, which
// the flaming arrows add. Exceptions, on purpose: the scorpion, a desert hunter used to heat, the water lotus, a
// wet stem that barely burns, and the ushabti, fired in a kiln (a unit test keeps the list: every new monster follows
// the rule or joins it).
constexpr Resistances WORM_RESIST = {RESISTS, WEAK, NORMAL, WEAK}; // soft: a blow squashes, a blade cuts, fire shrivels
constexpr Resistances SCARAB_RESIST =
	{NORMAL, RESISTS, WEAK, NORMAL}; // the shell turns a blade, a point goes between plates
constexpr Resistances PLANT_RESIST =
	{TOUGH, WEAK, TOUGH, WEAK}; // dry stems: a blade cuts, fire catches; points, blows go astray
constexpr Resistances BAT_RESIST =
	{WEAK, RESISTS, TOUGH, WEAK}; // swat it; an arrow goes through the wing, and thin membranes burn
constexpr Resistances MIMIC_RESIST =
	{WEAK, RESISTS, TOUGH, WEAK}; // wood: crack it; a point only sticks in it, and dry wood burns
constexpr Resistances ANUBIS_RESIST =
	{WEAK, RESISTS, NORMAL, TOUGH}; // bronze dents, a blade glances off, stone does not burn
constexpr Resistances MUMMY_RESIST = {RESISTS, WEAK, TOUGH, WEAK}; // dry linen tears and burns; nothing inside to stab
constexpr Resistances CROCODILE_RESIST =
	{NORMAL, RESISTS, NORMAL, RESISTS}; // the scutes turn a blade, wet hide a flame
constexpr Resistances SCORPION_RESIST =
	{WEAK, NORMAL, RESISTS, NORMAL}; // a blow cracks the thin shell; a point glances off; fire: the exception
constexpr Resistances COBRA_RESIST =
	{RESISTS, WEAK, NORMAL, NORMAL}; // the coils give under a blow; a blade cuts the body
constexpr Resistances EGG_CLUSTER_RESIST =
	{NORMAL, WEAK, RESISTS, WEAK}; // a blade slits the leathery eggs, fire cooks them
constexpr Resistances LOTUS_RESIST =
	{RESISTS, WEAK, TOUGH, RESISTS}; // a blade cuts the soft stem; blows and points bend it, wet it barely burns
constexpr Resistances CAMEL_SPIDER_RESIST =
	{NORMAL, WEAK, RESISTS, WEAK}; // a blade lops the long legs, a point misses the slim body; fire
constexpr Resistances HIPPO_RESIST =
	{RESISTS, NORMAL, NORMAL, RESISTS}; // a blow sinks into the fat, wet hide barely burns
// Glazed faience shatters under a blow; a point skids off the glaze; fired in a kiln, a flame only warms it (an
// exception to the fire rule).
constexpr Resistances USHABTI_RESIST = {WEAK, NORMAL, RESISTS, TOUGH};
constexpr Resistances BABOON_RESIST =
	{NORMAL, NORMAL, WEAK, RESISTS}; // lean, an arrow finds it; it lives by the lake of fire

// Chance a poisoning does not take (plan monster-poison). The rest have none, like the player without an amulet.
// Immune: no blood to carry it (the dead, bronze, a stem, wood, a nest of eggs).
constexpr int POISON_IMMUNE = 100;
// The Anubis's bolt leaves the sceptre head at frame 12 of 26 of its swing, as it points forward: 0.44 of its height,
// 2.6 half widths ahead of its centre (tools/blender/models/anubis.py --measure).
constexpr float ANUBIS_RELEASE = 12.f / 25.f;
constexpr float ANUBIS_RELEASE_Y = 0.44f;
constexpr float ANUBIS_RELEASE_AHEAD = 2.6f;
// The mummy priest's bolt leaves the staff head at frame 12 of 26 of his swing, pointing straight ahead
// (tools/blender/models/mummy_priest.py --measure).
constexpr float MUMMY_PRIEST_RELEASE = 12.f / 25.f;
constexpr float MUMMY_PRIEST_RELEASE_Y = 0.52f;
constexpr float MUMMY_PRIEST_RELEASE_AHEAD = 3.03f;
// The baboon's coal leaves its hand at frame 8 of 14 of its overarm throw, reared up, the hand at head height ahead
// of it (tools/blender/models/baboon.py --measure). Over its head for the screech before: the tell.
constexpr float BABOON_RELEASE = 0.615f;
constexpr float BABOON_RELEASE_Y = 1.371f;
constexpr float BABOON_RELEASE_AHEAD = 0.589f;
// Babi's (baboon.py --boss --measure): the same throw, his heavier mane makes the reference a little taller.
constexpr float BABI_RELEASE = 0.615f;
constexpr float BABI_RELEASE_Y = 1.308f;
constexpr float BABI_RELEASE_AHEAD = 0.589f;
constexpr int POISONER_RESIST = 50; // scorpions and cobras: half used to venom
// Spit in the eyes (plan giant-cobra-blinding-spit): the giant cobra blinds with half its spit hits, Apep with each.
constexpr int GIANT_COBRA_BLIND_MS = 2000;
constexpr int GIANT_COBRA_BLIND_PERCENT = 50;
constexpr int APEP_BLIND_MS = 3000;
// The screech (plan giant-bat-screech): the giant bat's; Shezmu's wider, farther, longer, sooner, and he drinks the
// dazed player's blood whole.
constexpr ScreechRules GIANT_BAT_SCREECH = {4.f, 60.f, 600, 1500, 6000, 0};
constexpr ScreechRules SHEZMU_SCREECH = {5.f, 90.f, 600, 2000, 5000, 100};
constexpr int POISONER_BOSS_RESIST = 75; // the scorpion queen, Apep: more than their kin

// Holds (HoldRules; plans crocodile-hold-bite, giant-scorpion-grab). In half water a crocodile's bite may grab: the
// bite's damage, then the death roll, 3 ticks of 40% of it, 0.8 s apart; no breaking free. Sobek's (its kin's,
// AGENTS.md Boss abilities): more often, a tick longer. The attack clip (tools/blender/models/crocodile.py): the gape
// at frame 8, the jaws shut at 13.
constexpr HoldRules CROCODILE_HOLD = {
	.chancePercent = 30,
	.inWater = true,
	.holdMs = 2400,
	.tickMs = 800,
	.tickDamagePct = 40,
	.tellFrame = 8.f,
	.holdFrame = 13.f
};
constexpr HoldRules SOBEK_HOLD = {
	.chancePercent = 40,
	.inWater = true,
	.holdMs = 3200,
	.tickMs = 800,
	.tickDamagePct = 40,
	.tellFrame = 8.f,
	.holdFrame = 13.f
};
// The giant scorpion's pincers: a small blunt hit, a 1 s hold, then the sure sting; each hit on it ends the hold
// 0.25 s sooner. The scorpion queen's (her kin's): longer, harder to break. The attack clip
// (tools/blender/models/scorpion.py): the claws wide and the tail up at frame 3, pinched and cocked at 8.
constexpr DamageMix PINCER_MIX = {100, 0, 0};
constexpr HoldRules GIANT_SCORPION_HOLD = {
	.chancePercent = 25,
	.grabDamagePct = 30,
	.grabMix = PINCER_MIX,
	.holdMs = 1000,
	.breakMs = 250,
	.sting = true,
	.tellFrame = 3.f,
	.holdFrame = 8.f
};
constexpr HoldRules SCORPION_QUEEN_HOLD = {
	.chancePercent = 25,
	.grabDamagePct = 30,
	.grabMix = PINCER_MIX,
	.holdMs = 1500,
	.breakMs = 200,
	.sting = true,
	.tellFrame = 3.f,
	.holdFrame = 8.f
};
// Rerek's swallow (plan worm-boss): the jaws close round the player and pull them into the sand; a grinding tick each
// second; each hit on him ends it 0.3 s sooner. The attack clip (tools/blender/models/worm.py ATTACK): reared, the
// jaws wide at frame 9, lunged with the jaws round the player at 13.
constexpr HoldRules REREK_HOLD = {
	.chancePercent = 30,
	.grabDamagePct = 60,
	.holdMs = 3000,
	.tickMs = 1000,
	.tickDamagePct = 35,
	.breakMs = 300,
	.swallow = true,
	.tellFrame = 9.f,
	.holdFrame = 13.f
};

// The dung ball (plan scarab-dung-ball). The draft's 4-6 tiles would kick it off the screen: the view shows about
// 1.8 tiles either side of the player (see the baboon), and the roller, its back to them, must stand in it with its
// head. So it rolls from 0.6 to 1 tile, slower than the draft's 4 tiles/s: the windup is the tell, a jump at the kick
// clears it. The windup is the roll clip's kick (tools/blender/models/scarab.py). Khepri, who rolls the sun across the
// sky, rolls a bigger, faster, burning one from a little farther (the boss rule, though her kin, the small scarab,
// rolls none); her bigger body leaves no more room in the view.
constexpr RollRules GIANT_SCARAB_ROLL = {
	.minRange = 0.6f,
	.maxRange = 1.f,
	.windupMs = 800,
	.cooldownMs = 4000,
	.speed = 2.2f,
	.radius = 0.1f,
	.damage = 11
};
constexpr RollRules KHEPRI_ROLL = {
	.minRange = 0.6f,
	.maxRange = 1.1f,
	.windupMs = 800,
	.cooldownMs = 3500,
	.speed = 2.8f,
	.radius = 0.13f,
	.damage = 12,
	.mix = {60, 0, 0, 40},
	.burnMs = 2000
};

// The pounce (plan monster-balance): the giant rat and the giant scarab leap at the player from 2-3 tiles, so the bow
// alone does not keep them off. The rat lands in bite reach; the scarab from just beyond its dung ball's band into it,
// the ball's windup the tell (from 2 tiles, a player backing off at a walk kept the slow scarab between the two, out of
// both: the balance sim). Khepri (the boss rule) from farther, more often.
constexpr PounceRules GIANT_RAT_POUNCE = {.minGap = 2.f, .maxGap = 3.f, .cooldownMs = 3000};
constexpr PounceRules GIANT_SCARAB_POUNCE = {.minGap = 1.2f, .maxGap = 3.f, .cooldownMs = 3000, .landGap = 0.8f};
constexpr PounceRules KHEPRI_POUNCE = {.minGap = 1.2f, .maxGap = 3.5f, .cooldownMs = 2500, .landGap = 1.05f};

// A boss has no weakness: where its kin is weak, it takes the damage normally.
constexpr Resistances bossResist(Resistances kin) {
	for (int& rate : kin)
		rate = rate > NORMAL ? NORMAL : rate;
	return kin;
}

// By MonsterTypeId, from 1. Small ones are quick and bite often but barely hurt; big ones are slow, hit hard and take
// long to kill. Flyers roost on the ceiling and swoop through the player (Monster::Fly). HP and damage (plan
// monster-balance, 2026-10-26): each tuned with the balance sim (bin/balance) against the player halfway through its
// first zone of the rebuilt campaign (campaign.h ZONE_FAUNA), a boss against the player of its level with up to four
// large health potions; deeper down a monster is easier. A weak one costs that player about 6% of their HP, a giant
// 20%, a boss two or three potions.
constexpr std::array<MonsterKind, MONSTER_TYPE_MAX> KINDS = {{
	{.id = MonsterScarab,
	 .glyph = 's',
	 .name = "Scarab",
	 .label = "scarab",
	 .description = "Scarab",
	 .threat = 0.8f,
	 .model = "monsters/scarab",
	 .texture = "monsters/scarab",
	 .scale = 7,
	 .blood = SCARAB_BLOOD,
	 .speed = 4,
	 .maxHealth = 14,
	 .damage = 4,
	 .attackMs = 600,
	 .xp = 300,
	 .attackMix = SCARAB_MIX,
	 .resist = SCARAB_RESIST,
	 .weight = 1,
	 .generated = {1, 5, 4}},
	{.id = MonsterWorm,
	 .glyph = 'w',
	 .name = "Sand worm",
	 .label = "worm",
	 .description = "Sand worm; in sand it lies under the surface and comes up under the player",
	 .threat = 2.f,
	 .model = "monsters/worm",
	 .texture = "monsters/worm",
	 .scale = 18,
	 .rotA = 0,
	 .speed = 1,
	 .maxHealth = 200,
	 .damage = 14,
	 .attackMs = 1000,
	 .xp = 1200,
	 .attackMix = WORM_MIX,
	 .resist = WORM_RESIST,
	 .meleeCurses = {CurseEntry{Curse::Slowness, CurseGrade::Lesser, 15}},
	 .sandBurrow = SandBurrowRules{},
	 .generated = {3, 7, 2}},
	{.id = MonsterPlant,
	 .glyph = 'p',
	 .name = "Thorn acacia",
	 .label = "plant",
	 .description = "Thorn acacia, its bite and its sap poison (weak)",
	 .threat = 1.5f, // does not move
	 .model = "monsters/plant",
	 .texture = "monsters/plant",
	 .scale = 12,
	 .rotA = 0,
	 .blood = {0.1f, 0.4f, 0.1f},
	 .speed = 0,
	 .maxHealth = 30,
	 .damage = 4,
	 .attackMs = 800,
	 .xp = 500,
	 .attackMix = PLANT_MIX,
	 .resist = PLANT_RESIST,
	 .locomotion = Locomotion::Stationary,
	 .poison = PoisonTier::Weak,
	 .poisonChancePercent = 30,
	 .touch = TouchPoison{PoisonTier::Weak, 20},
	 .poisonResistPercent = POISON_IMMUNE,
	 .generated = {3, 10, 2}},
	// Reckless like the Anubis boss; its trap share is between the boss (10%) and the mummy (50%). Tuned for the
	// necropolis, not its first zone: the late zones' own monster; in the priests' quarter, rare, the zone's terror.
	{.id = MonsterAnubis,
	 .glyph = 'n',
	 .name = "Anubis living statue",
	 .label = "anubis",
	 .description = "Anubis living statue",
	 .threat = 10.f, // walks faster than the player (plan anubis-speed): only a sprint gets away; +1 for the bolt
	 .model = "monsters/anubis",
	 .texture = "monsters/anubis",
	 .scale = 19,
	 .blood = {0.55f, 0.52f, 0.46f}, // stone chips
	 .speed = 23, // 1.21 tiles a second, 120% of the player's walk (one point: MONSTER_SEEK_STEP each 80 ms step)
	 .maxHealth = 1300,
	 .damage = 140,
	 .attackMs = 1100,
	 .xp = 10000,
	 .attackMix = ANUBIS_MIX,
	 .melee = HIGH_BLOW,
	 .resist = ANUBIS_RESIST,
	 .courage = Courage::Reckless,
	 .trapDamagePct = 25,
	 .weight = 3,
	 .wading = Wading::Unaffected,
	 .waterSpeed = 1.f,
	 .poisonResistPercent = POISON_IMMUNE,
	 // A bolt of fire from the was-sceptre (plans anubis-ranged-attack, anubis-fire-bolt). The melee swing is its
	 // ranged clip.
	 .ranged =
		 RangedRules{
			 70,
			 std::nullopt,
			 1.4f,
			 3000,
			 ANUBIS_RELEASE,
			 ANUBIS_RELEASE_Y,
			 BOLT_FIRE,
			 ANUBIS_RELEASE_AHEAD,
			 ANUBIS_BOLT_MIX
		 },
	 .meleeCurses = {CurseEntry{Curse::Vulnerability, CurseGrade::Lesser, 15}},
	 .rangedCurses = {CurseEntry{Curse::Palsy, CurseGrade::Lesser, 20}},
	 .walkClipSpeed = 70, // twice the usual, so the feet do not slide (plan anubis-walk-clip-speed)
	 .climbs = true,
	 .climbRise = 0.351f, // tools/blender/models/anubis.py
	 .climbGrip = 0.119f,
	 .generated = {9, 10, 1}},
	{.id = MonsterRat,
	 .glyph = 't',
	 .name = "Rat",
	 .label = "rat",
	 .description = "Rat",
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
	 .meleeCurses = {CurseEntry{Curse::Disease, CurseGrade::Lesser, 10}},
	 .answersCall = true,
	 .climbs = true,
	 .climbRise = 1.068f, // tools/blender/models/rat.py
	 .climbGrip = 1.14f,
	 .generated = {1, 5, 6}},
	// Faster than the player's walk (~1.25 tiles/s): the bow alone does not keep it off (plan giant-rat-speed).
	{.id = MonsterGiantRat,
	 .glyph = 'T',
	 .name = "Giant rat",
	 .label = "giant rat",
	 .description = "Giant rat",
	 .threat = 3.f,
	 .model = "monsters/rat",
	 .texture = "monsters/rat_giant",
	 .scale = 42,
	 .speed = 24,
	 .maxHealth = 250,
	 .damage = 5,
	 .attackMs = 700,
	 .xp = 2000,
	 .attackMix = RAT_MIX,
	 .locomotion = Locomotion::WalkJump,
	 .wading = Wading::Unaffected,
	 .waterSpeed = 1.f,
	 .meleeCurses =
		 {CurseEntry{Curse::Disease, CurseGrade::Greater, 3}, CurseEntry{Curse::Disease, CurseGrade::Lesser, 15}},
	 .call = CallRules{RAT_CALL_RADIUS, RAT_CALL_BELOW_PCT}, // plan giant-rat-pack-call
	 .answersCall = true,
	 .pounce = GIANT_RAT_POUNCE,
	 .climbs = true,
	 .climbRise = 1.068f, // tools/blender/models/rat.py
	 .climbGrip = 1.14f,
	 .generated = {4, 10, 3}},
	{.id = MonsterBat,
	 .glyph = 'f',
	 .name = "Bat",
	 .label = "bat",
	 .description = "Bat",
	 .threat = 1.2f, // weak, but only hit with good timing
	 .model = "monsters/bat",
	 .texture = "monsters/bat",
	 .scale = 18,
	 .speed = 5,
	 .maxHealth = 8,
	 .damage = 4,
	 .attackMs = 800,
	 .xp = 400,
	 .attackMix = BAT_MIX,
	 .resist = BAT_RESIST,
	 .locomotion = Locomotion::Fly,
	 .weight = 0,
	 .meleeCurses = {CurseEntry{Curse::Fatigue, CurseGrade::Lesser, 10}},
	 .generated = {2, 6, 3}},
	{.id = MonsterGiantBat,
	 .glyph = 'F',
	 .name = "Giant bat",
	 .label = "giant bat",
	 .description = "Giant bat, screeches: the player in its cone is dazed",
	 .threat = 3.5f,
	 .model = "monsters/bat",
	 .texture = "monsters/bat_giant",
	 .scale = 30,
	 .blood = {0.45f, 0.05f, 0.05f},
	 .speed = 4,
	 .maxHealth = 40,
	 .damage = 28,
	 .attackMs = 800,
	 .xp = 1800,
	 .attackMix = BAT_MIX,
	 .resist = BAT_RESIST,
	 .locomotion = Locomotion::Fly,
	 .weight = 0,
	 .meleeCurses = {CurseEntry{Curse::Fatigue, CurseGrade::Lesser, 15}},
	 .screech = GIANT_BAT_SCREECH,
	 .generated = {6, 10, 2}},
	// Scale and yaw of the treasure chest item: idle, it looks just like one.
	{.id = MonsterMimic,
	 .glyph = 'M',
	 .name = "Chest of Set",
	 .label = "mimic",
	 .description = "Chest of Set, a treasure chest until the player comes near",
	 .threat = 2.f, // hits hard, but does not move
	 .model = "monsters/mimic",
	 .texture = "monsters/mimic",
	 .scale = 8,
	 .rotA = 0,
	 .blood = {0.5f, 0.05f, 0.1f},
	 .speed = 0,
	 .maxHealth = 50,
	 .damage = 6,
	 .attackMs = 800,
	 .xp = 900,
	 .attackMix = MIMIC_MIX,
	 .resist = MIMIC_RESIST,
	 .locomotion = Locomotion::Ambush,
	 .poisonResistPercent = POISON_IMMUNE},
	{.id = MonsterGiantScarab,
	 .glyph = 'k',
	 .name = "Giant scarab",
	 .label = "giant scarab",
	 .description = "Giant scarab, leaps over pits and traps and at the player, rolls dung balls",
	 .threat = 4.f, // jumps like a giant rat, hits harder
	 .model = "monsters/scarab",
	 .texture = "monsters/scarab_giant",
	 .scale = 24,
	 .blood = {0.4f, 0.05f, 0.55f},
	 .speed = 3,
	 .maxHealth = 340,
	 .damage = 15,
	 .attackMs = 1000,
	 .xp = 2500,
	 .attackMix = SCARAB_MIX,
	 .resist = SCARAB_RESIST,
	 .locomotion = Locomotion::WalkJump,
	 .meleeCurses = {CurseEntry{Curse::Clumsiness, CurseGrade::Lesser, 15}},
	 .roll = GIANT_SCARAB_ROLL,
	 .pounce = GIANT_SCARAB_POUNCE,
	 .generated = {5, 10, 3}},
	// Bigger, faster and harder than the giant scarab; few scarabs, which take the player's blows (plan
	// monster-balance). Tuned for a level 6 player with the sword: the first zone must bring them there.
	{.id = MonsterBossScarab,
	 .glyph = 'K',
	 .name = "Khepri",
	 .label = "boss scarab",
	 .description = "Khepri, the boss scarab, summons scarabs, rolls burning dung balls; its death opens the boss "
					"gates. One boss per level",
	 .threat = 10.f, // a little weaker than an Anubis, plus its scarabs
	 .model = "monsters/scarab",
	 .texture = "monsters/scarab_boss",
	 .scale = 34,
	 .blood = {0.1f, 0.2f, 0.75f},
	 .speed = 3.5f,
	 .maxHealth = 400,
	 .damage = 8,
	 .attackMs = 900,
	 .xp = 6000,
	 .attackMix = SCARAB_MIX,
	 .resist = bossResist(SCARAB_RESIST),
	 .locomotion = Locomotion::WalkJump,
	 .weight = 4,
	 .meleeCurses =
		 {CurseEntry{Curse::Clumsiness, CurseGrade::Greater, 6}, CurseEntry{Curse::Clumsiness, CurseGrade::Lesser, 30}},
	 .roll = KHEPRI_ROLL,
	 .pounce = KHEPRI_POUNCE,
	 .kin = MonsterScarab,
	 .boss = {MonsterScarab, 2, 3, 1500, 12, 0, Summon::DigOut}},
	// A giant bat grown fat on blood.
	{.id = MonsterVampireBat,
	 .glyph = 'V',
	 .name = "Bat of Shezmu",
	 .label = "vampire bat",
	 .description = "Bat of Shezmu, summons bats, heals by part of the damage it deals (all of it on a dazed player); "
					"its screech dazes; its death opens the boss gates. One boss per level",
	 .threat = 12.f, // a giant bat that hits like a boss and heals, plus its bats
	 .model = "monsters/bat",
	 .texture = "monsters/bat_vampire",
	 .scale = 42,
	 .blood = {0.5f, 0.02f, 0.08f},
	 .speed = 4,
	 .maxHealth = 350,
	 .damage = 25,
	 .attackMs = 800,
	 .xp = 12000,
	 .attackMix = BAT_MIX,
	 .resist = bossResist(BAT_RESIST),
	 .locomotion = Locomotion::Fly,
	 .weight = 0,
	 .meleeCurses = {CurseEntry{Curse::Fatigue, CurseGrade::Lesser, 30}},
	 .screech = SHEZMU_SCREECH,
	 .kin = MonsterBat,
	 .boss = {MonsterBat, 2, 3, 2000, 8, 30, Summon::Drop}},
	// Fast for its bulk, hits hard and slowly. Levels 11 on, the mummy priest's minion (plan mummy-boss).
	{.id = MonsterMummy,
	 .glyph = 'u',
	 .name = "Mummy",
	 .label = "mummy",
	 .description = "Mummy, lies in its coffin until the player comes near",
	 .threat = 5.f, // walks fast for its size and hits hard, but gives the player time to see it climb out
	 .model = "monsters/mummy",
	 .texture = "monsters/mummy",
	 .scale = 18,
	 .blood = {0.35f, 0.25f, 0.12f},
	 .speed = 2.5f,
	 .maxHealth = 480,
	 .damage = 100,
	 .attackMs = 1600,
	 .xp = 2500,
	 .attackMix = MUMMY_MIX,
	 .melee = HIGH_BLOW,
	 .resist = MUMMY_RESIST,
	 .locomotion = Locomotion::Entombed,
	 .courage = Courage::Reckless, // a crushing rock (500) can kill it
	 .trapDamagePct = 50,
	 .poisonResistPercent = POISON_IMMUNE,
	 .meleeCurses =
		 {CurseEntry{Curse::Weakness, CurseGrade::Greater, 2}, CurseEntry{Curse::Weakness, CurseGrade::Lesser, 10}},
	 .riseMs = 1000,
	 .sunBurns = true,
	 .climbs = true,
	 .climbRise = 0.741f, // tools/blender/models/mummy.py
	 .climbGrip = 0.111f,
	 .generated = {7, 10, 2}},
	// The finale's guardian, a head taller than an Anubis, the strongest boss. Reckless: the player cannot shake him
	// off behind a row of traps.
	{.id = MonsterAnubisBoss,
	 .glyph = 'N',
	 .name = "Anubis",
	 .label = "anubis boss",
	 .description = "Anubis, the god himself, his living statues step out of the walls; his death opens the boss "
					"gates. One boss per level",
	 .threat = 16.f, // the finale, plus its guards and its bolt
	 .model = "monsters/anubis",
	 .texture = "monsters/anubis_boss",
	 .scale = 26,
	 .speed = 25, // 1.31 tiles a second, a little faster than its guard
	 .maxHealth = 2400,
	 .damage = 190,
	 .attackMs = 1300,
	 .xp = 30000,
	 .attackMix = ANUBIS_MIX,
	 .melee = HIGH_BLOW,
	 .resist = bossResist(ANUBIS_RESIST),
	 .courage = Courage::Reckless,
	 .trapDamagePct = 10,
	 .weight = 4,
	 .wading = Wading::Unaffected,
	 .waterSpeed = 1.f,
	 .poisonResistPercent = POISON_IMMUNE,
	 // Its guard's bolt, farther and harder, burning longer; the same model.
	 .ranged =
		 RangedRules{
			 125,
			 std::nullopt,
			 2.0f,
			 2500,
			 ANUBIS_RELEASE,
			 ANUBIS_RELEASE_Y,
			 BOLT_FIRE,
			 ANUBIS_RELEASE_AHEAD,
			 ANUBIS_BOLT_MIX,
			 ANUBIS_BOSS_BURN_MS
		 },
	 .meleeCurses = {CurseEntry{Curse::Vulnerability, CurseGrade::Lesser, 30}},
	 .rangedCurses = {CurseEntry{Curse::Palsy, CurseGrade::Lesser, 40}},
	 .walkClipSpeed = 76, // its walk is 25/23 of the guard's: the same stride (plan anubis-walk-clip-speed)
	 .climbs = true,
	 .climbRise = 0.351f, // tools/blender/models/anubis.py
	 .climbGrip = 0.119f,
	 .kin = MonsterAnubis,
	 // A guard is twice a mummy's threat and outruns the player: fewer, and slower to come (plan mummy-boss).
	 // Every other summon a servant, an ushabti, steps out of the stone (plan living-ushabti): four more summons and
	 // one more alive, so as many guards come as before (5).
	 .boss =
		 {.minion = MonsterAnubis,
		  .minAlive = 1,
		  .maxAlive = 3,
		  .summonMs = 4000,
		  .summonCap = 8,
		  .summon = Summon::Wall,
		  .smallMinion = MonsterUshabti}},
	// Slow on land (slower than a rat: the player outwalks it), in the water faster than the player walks on land
	// (plan crocodiles-and-flooded-cells). Tuned for the serpent temple, its first zone after the bat galleries' one
	// preview wade.
	// A long, low body: 1.5 tiles nose to tail.
	{.id = MonsterCrocodile,
	 .glyph = 'C',
	 .name = "Crocodile",
	 .label = "crocodile",
	 .description = "Crocodile, lies under the water until the player comes near, swims fast; in the water its bite "
					"may hold the player",
	 .threat = 4.5f, // between the giant rat and the mummy; in the water it outswims the player
	 .model = "monsters/crocodile",
	 .texture = "monsters/crocodile",
	 .scale = 60,
	 .speed = 7,
	 .maxHealth = 350,
	 .damage = 50,
	 .attackMs = 1100,
	 .xp = 2400,
	 .attackMix = CROCODILE_MIX,
	 .resist = CROCODILE_RESIST,
	 .locomotion = Locomotion::Submerged,
	 .weight = 3,
	 .wading = Wading::Swimmer,
	 .waterSpeed = 2.5f,
	 .hold = CROCODILE_HOLD},
	// Small and quick like the rat, a slower sting that poisons; levels from the scorpion queen's (plan
	// poison-and-antidote).
	{.id = MonsterScorpion,
	 .glyph = 'j',
	 .name = "Scorpion",
	 .label = "scorpion",
	 .description = "Scorpion, its sting poisons (weak)",
	 .threat = 1.f, // a little above the rat and the scarab: the poison burns on after the fight
	 .model = "monsters/scorpion",
	 .texture = "monsters/scorpion",
	 .scale = 15,
	 .blood = SCORPION_BLOOD,
	 .speed = 10,
	 .maxHealth = 150,
	 .damage = 30,
	 .attackMs = 900,
	 .xp = 450,
	 .attackMix = SCORPION_MIX,
	 .resist = SCORPION_RESIST,
	 .weight = 1,
	 .poison = PoisonTier::Weak,
	 .poisonResistPercent = POISONER_RESIST,
	 .climbs = true,
	 .climbRise = 0.932f, // tools/blender/models/scorpion.py
	 .climbGrip = 0.25f},
	// Between the giant rat and the crocodile, a poisoned bite and a venom spit from afar; lies coiled until the player
	// comes near (plan cobra).
	{.id = MonsterCobra,
	 .glyph = 'c',
	 .name = "Cobra",
	 .label = "cobra",
	 .description = "Cobra, lies coiled until the player comes near; its bite and spit poison (medium)",
	 .threat = 3.f, // a giant rat's, the poison on top, and it spits from afar
	 .model = "monsters/cobra",
	 .texture = "monsters/cobra",
	 .scale = 24,
	 .speed = 6,
	 .maxHealth = 330,
	 .damage = 50,
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
	 .ranged = RangedRules{10, PoisonTier::Medium, 1.2f, 3500, 0.41f, 0.87f}, // release: frame 7 of 18
	 .riseMs = 750},
	// The cobra's giant kin, levels after Apep (plan giant-cobra).
	{.id = MonsterGiantCobra,
	 .glyph = 'Q',
	 .name = "Giant cobra",
	 .label = "giant cobra",
	 .description =
		 "Giant cobra, lies coiled until the player comes near; its bite and spit poison (medium), its spit may blind",
	 .threat = 5.f, // between the giant scarab and the crocodile, the poison and the spit on top
	 .model = "monsters/cobra",
	 .texture = "monsters/cobra_giant",
	 .scale = 36,
	 .speed = 8,
	 .maxHealth = 690,
	 .damage = 85,
	 .attackMs = 1300,
	 .xp = 2600,
	 .attackMix = COBRA_MIX,
	 .resist = COBRA_RESIST,
	 .locomotion = Locomotion::Coiled,
	 .wading = Wading::Swimmer,
	 .waterSpeed = 1.25f,
	 .poison = PoisonTier::Medium,
	 .poisonResistPercent = POISONER_RESIST,
	 .ranged =
		 RangedRules{
			 .damage = 12,
			 .poison = PoisonTier::Medium,
			 .range = 1.5f,
			 .cooldownMs = 3000,
			 .release = 0.41f,
			 .releaseY = 0.87f,
			 .blindMs = GIANT_COBRA_BLIND_MS,
			 .blindPercent = GIANT_COBRA_BLIND_PERCENT
		 },
	 .meleeCurses = {CurseEntry{Curse::Slowness, CurseGrade::Lesser, 15}},
	 .riseMs = 750},
	// The scorpion's giant kin and the scorpion queen's minion: medium poison (plan scorpion-queen-boss).
	{.id = MonsterGiantScorpion,
	 .glyph = 'J',
	 .name = "Giant scorpion",
	 .label = "giant scorpion",
	 .description = "Giant scorpion, its claws may hold the player for a sure sting; its sting poisons (medium)",
	 .threat = 3.5f, // a giant rat's, the poison on top
	 .model = "monsters/scorpion",
	 .texture = "monsters/scorpion_giant",
	 .scale = 26,
	 .blood = SCORPION_BLOOD,
	 .speed = 9,
	 .maxHealth = 570,
	 .damage = 44,
	 .attackMs = 1000,
	 .xp = 2000,
	 .attackMix = SCORPION_MIX,
	 .resist = SCORPION_RESIST,
	 .poison = PoisonTier::Medium,
	 .poisonResistPercent = POISONER_RESIST,
	 .hold = GIANT_SCORPION_HOLD,
	 .climbs = true,
	 .climbRise = 0.932f, // tools/blender/models/scorpion.py
	 .climbGrip = 0.25f},
	// Rooted and harmless: the scorpion queen's brood hatches from it (Summon::Hatch). Dies to a few blows.
	{.id = MonsterEggCluster,
	 .glyph = 'e',
	 .name = "Egg cluster",
	 .label = "egg cluster",
	 .description = "Egg cluster, harmless; the scorpion queen's brood hatches from it",
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
	// The lvl14 boss on the scorpion model, the size of a cart (plan scorpion-queen-boss).
	{.id = MonsterScorpionQueen,
	 .glyph = 'U',
	 .name = "Scorpion queen",
	 .label = "scorpion queen",
	 .description = "Scorpion queen, giant scorpions hatch from the egg clusters in her room; her sting poisons "
					"(strong); her death opens the boss gates. One boss per level",
	 .threat = 13.f, // between the vampire bat and the anubis boss, plus her brood
	 .model = "monsters/scorpion",
	 .texture = "monsters/scorpion_queen",
	 .scale = 48,
	 .blood = {0.5f, 0.7f, 0.6f},
	 .speed = 5,
	 .maxHealth = 1800,
	 .damage = 30,
	 .attackMs = 1100,
	 .xp = 15000,
	 .attackMix = SCORPION_MIX,
	 .resist = bossResist(SCORPION_RESIST),
	 .courage = Courage::Reckless,
	 .trapDamagePct = 10,
	 .weight = 4,
	 .poison = PoisonTier::Strong,
	 .poisonResistPercent = POISONER_BOSS_RESIST,
	 .hold = SCORPION_QUEEN_HOLD,
	 .climbs = true,
	 .climbRise = 0.932f, // tools/blender/models/scorpion.py
	 .climbGrip = 0.25f,
	 .kin = MonsterScorpion,
	 .boss = {MonsterGiantScorpion, 2, 4, 3000, 10, 0, Summon::Hatch, MonsterEggCluster, MonsterScorpion}},
	// The lvl19 boss on the cobra model, long as the hall (plan apep-serpent-boss).
	{.id = MonsterApep,
	 .glyph = 'P',
	 .name = "Apep",
	 .label = "apep",
	 .description = "Apep, dives into the floor and comes up behind the player; spits a blinding fan of three; cobras "
					"dig out round "
					"him; his death "
					"opens the boss gates. One boss per level",
	 .threat = 14.f, // hard to pin down, plus his cobras
	 .model = "monsters/cobra",
	 .texture = "monsters/cobra_apep",
	 .scale = 70,
	 .blood = {0.35f, 0.05f, 0.1f},
	 .speed = 6,
	 .maxHealth = 1900,
	 .damage = 70,
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
	 // Between dives, a fan of three that blinds with each hit (plan giant-cobra-blinding-spit).
	 .ranged =
		 RangedRules{
			 .damage = 11,
			 .poison = PoisonTier::Medium,
			 .range = 2.5f,
			 .cooldownMs = 4000,
			 .release = 0.41f,
			 .releaseY = 0.87f,
			 .fan = 3,
			 .blindMs = APEP_BLIND_MS,
			 .blindPercent = 100
		 },
	 .meleeCurses = {CurseEntry{Curse::Slowness, CurseGrade::Lesser, 30}},
	 .kin = MonsterCobra,
	 .boss = {MonsterCobra, 2, 3, 3000, 10, 0, Summon::DigOut}},
	// The lvl29 boss on the crocodile model (plan sobek-boss).
	{.id = MonsterSobek,
	 .glyph = 'W',
	 .name = "Sobek",
	 .label = "sobek",
	 .description = "Sobek, lies in the water, charges along the row (a wall stuns him); crocodiles come out round "
					"him; his death opens the boss gates. One boss per level",
	 .threat = 14.5f, // a charge that hits like a cart, plus his crocodiles
	 .model = "monsters/crocodile",
	 .texture = "monsters/crocodile_sobek",
	 .scale = 100,
	 .speed = 6,
	 .maxHealth = 1400,
	 .damage = 50,
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
	 .charge = ChargeRules{},
	 .hold = SOBEK_HOLD,
	 .kin = MonsterCrocodile,
	 .boss = {MonsterCrocodile, 1, 1, 5000, 6, 0, Summon::DigOut}},
	// The blue lotus, rooted in half water among the decor lotus (plan water-lotus). Stronger than the plant, weaker
	// than the crocodile; its bite drains stamina, so the wading player cannot sprint away. Placed by hand on the
	// flooded levels from 7 on: levelgen makes no water.
	{.id = MonsterLotus,
	 .glyph = 'd',
	 .name = "Blue lotus",
	 .label = "lotus",
	 .description = "Blue lotus, a flower on the water until the player comes near; its bite drains stamina. Only in "
					"half water",
	 .threat = 2.f, // does not move, but the player wades to it and cannot sprint away
	 .model = "monsters/lotus",
	 .texture = "monsters/lotus",
	 .scale = 12, // 0.7 of its modelled size (lotus.py prints 16.8): reared, its head is at the wading player's face
	 .rotA = 0,
	 .blood = {0.35f, 0.5f, 0.85f}, // blue sap
	 .speed = 0,
	 .maxHealth = 250,
	 .damage = 90,
	 .attackMs = 900,
	 .xp = 1300,
	 .attackMix = LOTUS_MIX,
	 .resist = LOTUS_RESIST,
	 .locomotion = Locomotion::Floating,
	 .weight = 1,
	 .poisonResistPercent = POISON_IMMUNE,
	 .staminaDrainPct = 25,
	 .riseMs = 600},
	// The mummies' boss (plan mummy-boss): a high priest who read the curses at the funerals, a head taller than a
	// mummy. Lies in a large coffin; a staff for blows and for a purple bolt; curses more and nastier than any other.
	// lvl33 for now. The player comes there at about level 52: between Sobek and the Anubis boss.
	{.id = MonsterMummyPriest,
	 .glyph = 'I',
	 .name = "High Priest of Anubis",
	 .label = "mummy priest",
	 .description = "High Priest of Anubis, the mummy boss, lies in a large coffin; staff and a purple bolt, many "
					"curses; mummies climb out of the coffins round him; his death opens the boss gates. One boss per "
					"level",
	 .threat = 15.f, // between Sobek and the Anubis boss: the curses and the bolt, plus his mummies
	 .model = "monsters/mummy_priest",
	 .texture = "monsters/mummy_priest",
	 .scale = 20.5f,
	 .blood = {0.3f, 0.2f, 0.1f},
	 .speed = 3,
	 .maxHealth = 1700,
	 .damage = 35,
	 .attackMs = 1500,
	 .xp = 27000,
	 .attackMix = {80, 0, 20}, // the staff, its pointed foot
	 .melee = HIGH_BLOW,
	 .resist = bossResist(MUMMY_RESIST),
	 .locomotion = Locomotion::Entombed,
	 .courage = Courage::Reckless,
	 .trapDamagePct = 10,
	 .weight = 4,
	 .poisonResistPercent = POISON_IMMUNE,
	 .ranged =
		 RangedRules{
			 80,
			 std::nullopt,
			 1.8f,
			 2800,
			 MUMMY_PRIEST_RELEASE,
			 MUMMY_PRIEST_RELEASE_Y,
			 BOLT_PURPLE,
			 MUMMY_PRIEST_RELEASE_AHEAD
		 },
	 // The staff, the embalmer's work on the body: the rot the natron was meant to stop, his mummies' curse, the
	 // long sleep of the tomb. About 42% of the blows that land lay one.
	 .meleeCurses =
		 {CurseEntry{Curse::Disease, CurseGrade::Greater, 12},
		  CurseEntry{Curse::Weakness, CurseGrade::Greater, 12},
		  CurseEntry{Curse::Fatigue, CurseGrade::Lesser, 25}},
	 // The bolt, the spoken curses of the funeral rites: the forbidden words of the Book of the Dead, bound in linen
	 // like the dead, weighed against the feather and found wanting. About 58%.
	 .rangedCurses =
		 {CurseEntry{Curse::Insanity, CurseGrade::Lesser, 20},
		  CurseEntry{Curse::Slowness, CurseGrade::Lesser, 25},
		  CurseEntry{Curse::Vulnerability, CurseGrade::Lesser, 30}},
	 .riseMs = 1000,
	 .sunBurns = true,
	 .climbs = true,
	 .climbRise = 0.65f, // tools/blender/models/mummy_priest.py
	 .climbGrip = 0.118f,
	 .kin = MonsterMummy,
	 .boss = {MonsterMummy, 1, 1, 2000, 10, 0, Summon::Coffin}},
	// The camel spider of the soldiers' stories (plan camel-spider): faster than the giant rat, weak, a scissor bite.
	// It runs at the player in the dark and keeps out of the light of torches and braziers, so a lit stretch is a wall
	// to it. Early-middle levels, after the scorpions' sand.
	{.id = MonsterCamelSpider,
	 .glyph = 'h',
	 .name = "Camel spider",
	 .label = "camel spider",
	 .description = "Camel spider, fast; keeps out of the light of torches, braziers and lamps",
	 .threat = 2.5f, // fast and bites often, but weak, and the light keeps it off
	 .model = "monsters/camel_spider",
	 .texture = "monsters/camel_spider",
	 .scale = 28,
	 .blood = {0.55f, 0.6f, 0.35f}, // pale yellow-green haemolymph
	 .speed = 28,
	 .maxHealth = 330,
	 .damage = 5,
	 .attackMs = 550,
	 .xp = 1100,
	 .attackMix = CAMEL_SPIDER_MIX,
	 .resist = CAMEL_SPIDER_RESIST,
	 .weight = 1,
	 .walkClipSpeed = 60,
	 .shunsLight = true,
	 .generated = {3, 8, 2}},
	// The hippo (plan hippo), the deadliest animal of the Nile: lies under the water like the crocodile, comes out
	// charging along its row like Sobek, nearer and slower (ChargeRules). Heavy: more HP than the giant cobra, a
	// crushing bite. Slower than the crocodile on land, in the water faster than the wading player. The flooded levels
	// after the crocodile's, placed by hand: levelgen makes no water.
	{.id = MonsterHippo,
	 .glyph = 'o', // its eyes over the water
	 .name = "Hippopotamus",
	 .label = "hippo",
	 .description = "Hippo, lies under the water until the player comes near, then charges along the row (a wall "
					"stuns it); slow on land",
	 .threat = 6.f, // above the giant cobra: the charge in the water, where the player cannot jump it
	 .model = "monsters/hippo",
	 .texture = "monsters/hippo",
	 .scale = 52,
	 .speed = 4,
	 .maxHealth = 585,
	 .damage = 91,
	 .attackMs = 1500,
	 .xp = 3600,
	 .attackMix = HIPPO_MIX,
	 .resist = HIPPO_RESIST,
	 .locomotion = Locomotion::Submerged,
	 .weight = 4,
	 .wading = Wading::Swimmer,
	 .waterSpeed = 3.f,
	 .charge = ChargeRules{HIPPO_CHARGE_MIN, HIPPO_CHARGE_MAX, HIPPO_CHARGE_SPEED}},
	// The living ushabti (plan living-ushabti), the overseer of the servant figures buried to work for the dead: it
	// stands among the decor ushabti on its cell (scatterProps) until the player comes near. Small, slow, a few HP,
	// glazed faience: the club's. The tomb levels, where the decor ushabti are (DECOR_TIERS).
	{.id = MonsterUshabti,
	 .glyph = '@', // the free letters are taken by the level sources' defs
	 .name = "Living ushabti",
	 .label = "ushabti",
	 .description = "Living ushabti, stands still among the decor figures until the player comes near",
	 .threat = 1.5f, // slow and frail, but it hides among the decor
	 .model = "monsters/ushabti",
	 .texture = "monsters/ushabti",
	 .scale = 11, // 0.28 tiles tall, twice the decor figures, half the player
	 .rotA = 0,
	 .blood = {0.1f, 0.5f, 0.47f}, // faience chips
	 .speed = 6,
	 .maxHealth = 130,
	 .damage = 55,
	 .attackMs = 1300,
	 .xp = 900,
	 .attackMix = USHABTI_MIX,
	 .melee = HIGH_BLOW,
	 .resist = USHABTI_RESIST,
	 .locomotion = Locomotion::Figurine,
	 .weight = 1,
	 .poisonResistPercent = POISON_IMMUNE,
	 .riseMs = 900,
	 .generated = {6, 10, 1}},
	// The cursed mimic (plan cursed-mimic): the mimic's darker kin, a chest until woken like it, then it hops after
	// the player and leaps the pits (Locomotion::AmbushHop), a bit slower than their walk. The only source of insanity.
	// Hand-placed on levels 25-34 beside plain mimics; levelgen turns some of its mimics into it (CURSED_MIMIC_*).
	{.id = MonsterCursedMimic,
	 .glyph = 'x',
	 .name = "Cursed chest of Set",
	 .label = "cursed mimic",
	 .description = "Cursed chest of Set, a dark treasure chest until the player comes near; hops, its bite maddens",
	 .threat = 4.f, // hits hard, follows over pits, and insanity locks the map
	 .model = "monsters/mimic_cursed",
	 .texture = "monsters/mimic_cursed",
	 .scale = 10,
	 .rotA = 0,
	 .blood = {0.35f, 0.05f, 0.3f},
	 .speed = 20,
	 .maxHealth = 880,
	 .damage = 90,
	 .attackMs = 900,
	 .xp = 3500,
	 .attackMix = MIMIC_MIX,
	 .resist = MIMIC_RESIST,
	 .locomotion = Locomotion::AmbushHop,
	 .poisonResistPercent = POISON_IMMUNE,
	 .meleeCurses = {CurseEntry{Curse::Insanity, CurseGrade::Lesser, 25}}},
	// Seth, the red hippopotamus (plan hippo-boss), on the hippo model: the hippo's lurk and charge, farther and
	// faster, and his storm roar as a stun ends. A spear is his bane, as Horus's harpoon: pierce stays normal while he
	// resists blows. The boss of the desert chapter's oasis.
	{.id = MonsterSeth,
	 .glyph = 'L',
	 .name = "Seth",
	 .label = "seth",
	 .description = "Seth, the red hippo: lies in the water, charges along the row (a wall stuns him), roars a "
					"sandstorm as the stun ends; hippos come out round him; his death opens the boss gates. One boss "
					"per level",
	 .threat = 16.f, // Sobek's charge, farther and faster, the roar, and hippos
	 .model = "monsters/hippo",
	 .texture = "monsters/hippo_seth",
	 .sounds = "monsters/seth",
	 .scale = 88, // 1.7 x the hippo's, as Sobek on the crocodile
	 .blood = {0.45f, 0.05f, 0.02f},
	 .speed = 5,
	 .maxHealth = 2500,
	 .damage = 65,
	 .attackMs = 1500,
	 .xp = 30000,
	 .attackMix = HIPPO_MIX,
	 .resist = bossResist(HIPPO_RESIST),
	 .locomotion = Locomotion::Submerged,
	 .courage = Courage::Reckless,
	 .trapDamagePct = 10,
	 .weight = 6,
	 .wading = Wading::Swimmer,
	 .waterSpeed = 3.f,
	 .charge = ChargeRules{SETH_CHARGE_MIN, SETH_CHARGE_MAX, SETH_CHARGE_SPEED},
	 .roar = RoarRules{SETH_ROAR_RADIUS, SETH_ROAR_DAMAGE, SETH_ROAR_SHOVE, SETH_ROAR_HAZE_MS},
	 .kin = MonsterHippo,
	 .boss = {MonsterHippo, 1, 1, 7000, 4, 0, Summon::DigOut}},
	// The baboons of the Lake of Fire (plan baboon), Thoth's, at the gate of the Duat: the first kiter (plan
	// monster-kiting). It keeps the player at the range of its throw, backs off when they come at it and bites only
	// when cornered. The coal sets the player burning; water puts it out. Quick, a climber, mid HP.
	{.id = MonsterBaboon,
	 .glyph = 'z',
	 .name = "Baboon of the Lake of Fire",
	 .label = "baboon",
	 .description = "Baboon, keeps its distance and hurls burning coals (a screech before each); backs off when the "
					"player comes at it, bites when cornered; climbs",
	 .threat = 5.5f, // the giant cobra's, its coal burns from afar; the kiter's factor on top (KITER_THREAT_FACTOR)
	 .model = "monsters/baboon",
	 .texture = "monsters/baboon",
	 .scale = 24, // under half the player's height on all fours; KIND_SCALE in tools/blender/models/baboon.py
	 .speed = 22, // a bit faster than the player's walk: it comes up to its band
	 .maxHealth = 110,
	 .damage = 22,
	 .attackMs = 900,
	 .xp = 3200,
	 .attackMix = BABOON_MIX,
	 .resist = BABOON_RESIST,
	 .ranged =
		 RangedRules{
			 .damage = 24,
			 .range = 1.3f,
			 .cooldownMs = 2000,
			 .release = BABOON_RELEASE,
			 .releaseY = BABOON_RELEASE_Y,
			 .colour = BOLT_FIRE,
			 .releaseAhead = BABOON_RELEASE_AHEAD,
			 .mix = BABOON_COAL_MIX,
			 .ownClip = true
		 },
	 .kite = KiteRules{.near = 0.55f, .far = 1.1f, .backOffSpeed = 28.f},
	 .walkClipSpeed = 84, // its trot covers 0.48 tiles a second at the usual 35
	 .climbs = true,
	 .climbRise = 0.5539f, // tools/blender/models/baboon.py
	 .climbGrip = 0.0895f,
	 .generated = {9, 10, 1}},
	// Rerek, the sand serpent of Sokar's land (plan worm-boss), on the worm model: the worm's sand burrowing, farther
	// and faster, coming up past the player every other time in a spray of sand, and a bite that swallows. Out of sand
	// he walks, slowly. The boss of the land of Sokar (campaign zone 9), no campaign slot yet.
	{.id = MonsterRerek,
	 .glyph = 'q',
	 .name = "Rerek",
	 .label = "rerek",
	 .description = "Rerek, the sand serpent: in sand lies under the surface, comes up under or past the player in a "
					"spray of sand, swallows; worms dig out round him; his death opens the boss gates. One boss per "
					"level",
	 .threat = 16.5f, // hard to pin down, his swallow, and worms under the sand
	 .model = "monsters/worm",
	 .texture = "monsters/worm_rerek",
	 .sounds = "monsters/rerek",
	 .scale = 45, // 2.5 x the worm's
	 .rotA = 0,
	 .speed = 3,
	 .maxHealth = 3400,
	 .damage = 190,
	 .attackMs = 1300,
	 .xp = 32000,
	 .attackMix = WORM_MIX,
	 .resist = bossResist(WORM_RESIST),
	 .courage = Courage::Reckless,
	 .trapDamagePct = 10,
	 .weight = 5,
	 .meleeCurses = {CurseEntry{Curse::Slowness, CurseGrade::Lesser, 30}},
	 .spray = RoarRules{REREK_SPRAY_RADIUS, 0, REREK_SPRAY_SHOVE, REREK_SPRAY_HAZE_MS},
	 .hold = REREK_HOLD,
	 .sandBurrow =
		 SandBurrowRules{REREK_WAKE_RANGE, REREK_DIVE_MIN, REREK_DIVE_RANGE, REREK_UNDER_MS, REREK_EVERY_MS, true},
	 .kin = MonsterWorm,
	 .boss = {MonsterWorm, 2, 4, 5000, 8, 0, Summon::DigOut}},
	// The Mother of Webs (plan camel-spider-boss), on the camel spider's model: a beast, not a god (Egypt had no spider
	// god); the webs in the halls are hers. The camel spider's shunning of the light, stronger: faster, and she puts
	// out the torches and lamps she passes, so the dark spreads as the fight goes on; only the braziers stay lit. She
	// spins webs over the floor that slow the player, and her spiders drop from the ceiling. The web halls' boss
	// (campaign zone 3), no campaign slot yet.
	{.id = MonsterMotherOfWebs,
	 .glyph = ')',
	 .name = "Mother of Webs",
	 .label = "mother of webs",
	 .description = "Mother of Webs, the camel spiders' boss: fast, keeps out of the light, puts out the torches and "
					"lamps she passes (not the braziers), spins webs on the floor that slow the player; camel spiders "
					"drop round her; her death opens the boss gates. One boss per level",
	 .threat = 12.f, // fast in the dark she makes, her webs hold the player, and her spiders
	 .model = "monsters/camel_spider",
	 .texture = "monsters/camel_spider_mother",
	 .sounds = "monsters/mother",
	 .scale = 70, // 2.5 x the camel spider's
	 .blood = {0.42f, 0.48f, 0.26f},
	 .speed = 34,
	 .maxHealth = 800,
	 .damage = 10,
	 .attackMs = 600,
	 .xp = 9000,
	 .attackMix = CAMEL_SPIDER_MIX,
	 .resist = bossResist(CAMEL_SPIDER_RESIST),
	 .weight = 4,
	 .walkClipSpeed = 60,
	 .shunsLight = true,
	 .douseCells = MOTHER_DOUSE_CELLS,
	 .webs = WebRules{MOTHER_WEB_EVERY_MS, MOTHER_WEB_MAX},
	 .kin = MonsterCamelSpider,
	 .boss = {MonsterCamelSpider, 2, 2, 3000, 10, 0, Summon::Drop}},
	// Babi (plan babi-boss), the bull of the baboons, who eats the hearts of the dead by the lake of fire: a god, like
	// Seth. The baboon's kiting and throw, stronger: a wider band and a quicker retreat, leaping the gaps between the
	// ledges of his arena where the baboon would be cornered; volleys of coals, and every third throw a big one that
	// sets the floor burning. Cornered, a two-handed blow and a heavy bite. The Lake of Fire's boss (campaign zone
	// 10), no campaign slot yet.
	{.id = MonsterBabi,
	 .glyph = ']',
	 .name = "Babi",
	 .label = "babi",
	 .description =
		 "Babi, the baboons' boss: keeps his distance and throws volleys of burning coals, every third throw a "
		 "big one that sets the floor burning; backs off fast and leaps over gaps; baboons climb out round "
		 "him; his death opens the boss gates. One boss per level",
	 .threat = 16.f, // between Seth and the Anubis boss: his volleys from afar, the burning floor, and baboons
	 .model = "monsters/babi",
	 .texture = "monsters/babi",
	 .scale = 48, // twice the baboon's; KIND_SCALE of tools/blender/models/baboon.py --boss
	 .blood = {0.55f, 0.08f, 0.06f},
	 .speed = 26,
	 .maxHealth = 1500,
	 .damage = 45,
	 .attackMs = 1200,
	 .xp = 30000,
	 .attackMix = BABI_MIX,
	 .resist = bossResist(BABOON_RESIST),
	 .locomotion = Locomotion::WalkJump,
	 .courage = Courage::Reckless,
	 .trapDamagePct = 10,
	 .weight = 5,
	 .ranged =
		 RangedRules{
			 .damage = 30,
			 .range = 1.4f,
			 .cooldownMs = 1600,
			 .release = BABI_RELEASE,
			 .releaseY = BABI_RELEASE_Y,
			 .colour = BOLT_FIRE,
			 .releaseAhead = BABI_RELEASE_AHEAD,
			 .mix = BABOON_COAL_MIX,
			 .ownClip = true,
			 .volley = VolleyRules{2, 3, 250, 3, 90, 4000}
		 },
	 .kite = KiteRules{.near = 0.6f, .far = 1.25f, .backOffSpeed = 40.f},
	 .walkClipSpeed = 50, // the baboon's trot at twice the size: 84 * 26 / 22 / 2
	 .climbs = true,
	 .climbRise = 0.2641f, // tools/blender/models/baboon.py --boss (KIND_SCALE 48)
	 .climbGrip = 0.0427f,
	 .kin = MonsterBaboon,
	 .boss = {MonsterBaboon, 2, 2, 5000, 8, 0, Summon::DigOut}},
}};

constexpr bool validKinds() {
	for (size_t i = 0; i < KINDS.size(); i++) {
		const MonsterKind& kind = KINDS[i];
		if (kind.id != static_cast<int>(i) + 1 ||
			kind.attackMix[0] + kind.attackMix[1] + kind.attackMix[2] + kind.attackMix[3] != 100)
			return false;
		if (kind.ranged && kind.ranged->mix) {
			const DamageMix& mix = *kind.ranged->mix;
			if (mix[0] + mix[1] + mix[2] + mix[3] != 100)
				return false;
		}
		if (kind.boss.minion < 0 || kind.boss.minion > MONSTER_TYPE_MAX || kind.boss.smallMinion < 0 ||
			kind.boss.smallMinion > MONSTER_TYPE_MAX || (kind.boss.summon == Summon::Hatch) != (kind.boss.nest != 0))
			return false;
		if (kind.isBoss() != (kind.kin != 0) || kind.kin < 0 || kind.kin > MONSTER_TYPE_MAX)
			return false;
		if (kind.isBoss() && KINDS[static_cast<size_t>(kind.kin - 1)].sandBurrow && !kind.sandBurrow)
			return false; // the boss rule: its kin's sand burrowing too (AGENTS.md)
		if (kind.isBoss() && KINDS[static_cast<size_t>(kind.kin - 1)].shunsLight && !kind.shunsLight)
			return false; // and its shunning of the light
		if (kind.isBoss() && KINDS[static_cast<size_t>(kind.kin - 1)].kite && !kind.kite)
			return false; // and its kiting
		if (kind.ranged && kind.ranged->volley &&
			(kind.ranged->volley->minCount < 1 || kind.ranged->volley->maxCount < kind.ranged->volley->minCount))
			return false;
		if (kind.poisonResistPercent < 0 || kind.poisonResistPercent > POISON_IMMUNE ||
			(kind.isBoss() && kind.poisonResistPercent < KINDS[static_cast<size_t>(kind.kin - 1)].poisonResistPercent))
			return false;
		if (kind.isBoss()) { // a boss reaches at least as far as its kin (Monster::Reach makes it bigger still)
			const MeleeReach& kin = KINDS[static_cast<size_t>(kind.kin - 1)].melee;
			if (kind.melee.slack < kin.slack || kind.melee.halfAngleDeg < kin.halfAngleDeg)
				return false;
		}
		for (int rate : kind.resist)
			if (rate <= 0) // every type hurts
				return false;
		for (size_t t = 0; kind.isBoss() && t < kind.resist.size(); t++)
			if (kind.resist[t] > NORMAL || kind.resist[t] > KINDS[static_cast<size_t>(kind.kin - 1)].resist[t])
				return false;
	}
	return true;
}
static_assert(
	validKinds(),
	"KINDS: one row per MonsterTypeId in order, attack and ranged mixes of 100%, hatchers with a nest, "
	"bosses with a kin (its sand burrowing, light shunning and kiting too), no weakness and no resistance worse "
	"than the kin's, poison included, a reach no shorter than the kin's, volleys of at least one shot"
);
} // namespace

const MonsterKind* monsterKind(int type) {
	return type >= 1 && type <= MONSTER_TYPE_MAX ? &KINDS[static_cast<size_t>(type - 1)] : nullptr;
}
