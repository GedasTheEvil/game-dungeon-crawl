#ifndef JOURNAL_IDS_H
#define JOURNAL_IDS_H

#include "curses.h"
#include "damage.h"
#include "items.h"
#include "shrines.h"
#include "skills.h"
#include <optional>

// The ids the world reports for the archaeologist's notebook (journal.h): the creature moves and the field notes.
// Apart from the Journal class, so the code that only reports one (WorldEvent, the sim) does not include it
// (plan journal-ids-header).

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
	Roar, // a charger's storm roar as its stun ends (MonsterKind::roar)
	Kite, // a kiter backs off from the player to keep them at its range (MonsterKind::kite)
	RollBall, // a roller kicks a dung ball at the player (MonsterKind::roll)
	Douse, // a douser puts out a torch or a lamp (MonsterKind::douseCells)
	SpinWeb, // a spinner spins a web over the floor (MonsterKind::webs)
	Blaze, // a volley thrower's big coal sets the floor burning (VolleyRules)
	LeapAway, // a leaping kiter leaps back over a gap, out of the player's reach (plan babi-boss)
	Blind, // a spit in the eyes blinds the player (RangedRules::blindMs)
	Screech, // a flyer screeches, the player in its cone is dazed (MonsterKind::screech)
	CallPack, // a hurt giant rat squeals and the rats near come (MonsterKind::call)
	AvoidTerrain, // a monster steps out of a hostile terrain (MonsterKind::hostile, plan terrain-affinity)
};
constexpr int CREATURE_MOVE_COUNT = 30;

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
	EyeOfHorus, // Eye of Horus learnt (written on learning; was Sharp eye)
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
	Sand, // the first step into sand (plan desert-chapter)
	Quicksand, // the first time the player sinks in quicksand
	SunMirrors, // the first mirror turned or sun disc lit (plan sunlight-mirrors)
	TrappedChests, // the first trapped chest that went off (plan trapped-chests)
	TombCurses, // the first curse from an object: a chest's execration text, a smashed ushabti
	// The hour notes (plan campaign-texts): written on entering a zone, one per ZoneId in order (hourNote).
	HourFirst,
	HourSecond,
	HourThird,
	HourFourth,
	HourFifth,
	HourSixth,
	HourSeventh,
	HourEighth,
	HourNinth,
	HourTenth,
	HourEleventh,
	HourTwelfth,
	// The amulet notes (plan amulet-extras): one per AmuletType in order, written when the first amulet of the type is
	// found (amuletNote).
	AmuletFirst,
	AmuletLast = AmuletFirst + AMULET_TYPE_COUNT - 1,
	// Saved by number: new notes go at the end, after the rings' and the Shrine of Isis.
	Stun, // the first monster stunned (plan skill-stun)
	Rings, // the first ride on a ring platform (plan ring-transporter)
	Stargate, // the Stargate dialled the first time (plan stargate-chamber), with its XP
	DialDevice, // its dial device pressed
	Coffins, // the first coffin opened (plan tomb-curse-containers)
	Sandstorm, // the first gust in a sandstorm hall (plan sandstorm-halls)
	// The ring notes (plan ring-accessories): one per RingGem in order, written when the first ring of the gem is found
	// (ringNote).
	RingFirst,
	RingLast = RingFirst + RING_GEM_COUNT - 1,
	ShrineOfIsis, // the first prayer at a Shrine of Isis (plan shrine-of-isis)
	// Plan journal-field-notes-gaps: a rhythm trap seen at work (out or about to be, in view), each skill written on
	// learning.
	WallSpikes,
	FireJets,
	PoisonDrips,
	Dodge,
	PowerHit,
	FlamingArrows,
	WordsOfThoth,
	// Plan heka-surplus: the new skills, written on learning, in SkillId order.
	KaOfOsiris,
	StrengthOfMontu,
	DjedOfPtah,
	BreathOfShu,
	SpeedOfMafdet,
	WisdomOfSeshat,
	DisarmTrap,
	QuiverOfNeith,
	// Plan shrines-of-the-gods: the first prayer at each god's shrine, in God order after Isis (shrineNote).
	ShrineOfMontu,
	ShrineOfPtah,
	ShrineOfAmun,
	ShrineOfSerket,
	SkyIron, // the first sky-iron ring (plan sky-iron-rings)
	PressurePlates, // the first plate gate a pressure plate opened (plan pressure-plate)
	FloorSpikes, // the first time in a spike or death trap (plan journal-trap-notes)
	RockFalls, // the first rock or sand fall set off by the player (plan journal-trap-notes)
	CeilingTraps, // the first jump into ceiling spikes (plan journal-trap-notes)
	// Plan field-notes-gates-and-water: the first of each.
	RiddleGates, // a riddle gate's scroll opened
	NameGates, // a sealed name gate touched
	Wading, // a step in half water
	TheDark, // a light shunner (camel or tiny spider) kept out of the light, in view
	// Plan weighing-of-the-heart: the verdict of the scales on the last level (judgment.h), one of the two.
	HeartLight,
	HeartHeavy,
	Bleeding, // the first time the player bleeds (plan bleeding)
	Bleed, // the Bleed skill learnt (plan skill-bleed)
	SharpArrows, // the skills of plan archer-skills learnt
	PoisonArrows,
	PoisonBlade,
	TouchOfImhotep, // plan skill-regeneration
};
constexpr int FIELD_NOTE_COUNT = 94 + AMULET_TYPE_COUNT + RING_GEM_COUNT;
// The hour note of a zone (ZoneId as an int; isZone).
[[nodiscard]] constexpr FieldNote hourNote(int zone) {
	return static_cast<FieldNote>(static_cast<int>(FieldNote::HourFirst) + zone);
}
// The field note written when the skill is learnt, none for a skill without one.
[[nodiscard]] std::optional<FieldNote> skillNote(SkillId skill);
// The note of a damage type, of a potion's group.
[[nodiscard]] FieldNote damageNote(DamageType type);
[[nodiscard]] FieldNote potionNote(ItemKind potion);
[[nodiscard]] constexpr FieldNote amuletNote(AmuletType type) {
	return static_cast<FieldNote>(static_cast<int>(FieldNote::AmuletFirst) + static_cast<int>(type));
}
[[nodiscard]] constexpr FieldNote ringNote(RingGem gem) {
	return static_cast<FieldNote>(static_cast<int>(FieldNote::RingFirst) + static_cast<int>(gem));
}
// The note of the first prayer at the god's shrine.
[[nodiscard]] constexpr FieldNote shrineNote(God g) {
	if (g == God::Isis)
		return FieldNote::ShrineOfIsis;
	return static_cast<FieldNote>(static_cast<int>(FieldNote::ShrineOfMontu) + static_cast<int>(g) - 1);
}
[[nodiscard]] constexpr FieldNote curseNote(Curse c) {
	return static_cast<FieldNote>(static_cast<int>(FieldNote::CurseWeakness) + static_cast<int>(c));
}

#endif
