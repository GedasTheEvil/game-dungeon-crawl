#ifndef LOOT_H
#define LOOT_H

#include "containers.h"
#include "curses.h"
#include "items.h"
#include "rng.h"
#include <array>
#include <optional>
#include <vector>

// Contents of one treasure chest on campaign level `level`: the item placed in the map first, then random bonus items,
// from `rng` (the game's gameplay stream, which a scenario seeds). A weapon chest's bonus weapon is a weaker one the
// player holds (`owned`). Any chest but an amulet's or a ring's may hold a lesser or minor amulet (RollChestAmulet),
// then a ring (RollChestRing).
std::vector<ItemKind> RollChestLoot(ItemKind placed, const OwnedWeapons& owned, int level, Rng& rng);

// Percent of the chests that hold an extra amulet; minor ones only from MINOR_AMULET_LEVEL on, then MINOR_AMULET_CHANCE
// percent of them.
constexpr int CHEST_AMULET_CHANCE = 10;
constexpr int MINOR_AMULET_LEVEL = 11;
constexpr int MINOR_AMULET_CHANCE = 40;
// A chest's extra amulet, if the roll gives one: lesser or minor, any type that has those tiers.
std::optional<ItemKind> RollChestAmulet(int level, Rng& rng);

// Percent of the chests that hold a ring (plan ring-accessories), rolled after the amulet. The metal by depth: copper
// from the start, silver from SILVER_RING_LEVEL on at SILVER_RING_CHANCE, gold from GOLD_RING_LEVEL on at
// GOLD_RING_CHANCE (rolled first). The amethyst, the strongest, at half the weight of the other gems.
constexpr int CHEST_RING_CHANCE = 8;
constexpr int SILVER_RING_LEVEL = MINOR_AMULET_LEVEL;
constexpr int SILVER_RING_CHANCE = 40;
constexpr int GOLD_RING_LEVEL = 30;
constexpr int GOLD_RING_CHANCE = 15;
constexpr int RING_GEM_WEIGHT = 2;
constexpr int AMETHYST_WEIGHT = 1;
// A ring for level `level`: its metal by depth, its gem by weight.
ItemKind RollRing(int level, Rng& rng);
std::optional<ItemKind> RollChestRing(int level, Rng& rng);

// A boss's amulet: normal or grand (grand at BOSS_GRAND_PERCENT_PER_LEVEL per level, 10% on level 5), any type.
constexpr int BOSS_GRAND_PERCENT_PER_LEVEL = 2;
ItemKind RollBossAmulet(int level, Rng& rng);

// The item in the chest a killed mimic leaves: a weapon the player holds or any potion but the antidote, even odds for
// weapon or potion. The club, if nothing is held.
ItemKind RollMimicLoot(const OwnedWeapons& owned, Rng& rng);

// The cursed mimic's chest (plan cursed-mimic), a grade better: CURSED_MIMIC_AMULET_CHANCE percent a lesser or minor
// amulet (as RollChestAmulet's on level `level`), else ringPercent a ring (RollRing), else the mimic's loot.
constexpr int CURSED_MIMIC_AMULET_CHANCE = 50;
ItemKind RollCursedMimicLoot(const OwnedWeapons& owned, int level, Rng& rng, int ringPercent = 0);

// Percent of the kills (minions and mimics excepted) that leave a weapon chest; a boss always leaves an amulet chest.
constexpr int KILL_DROP_CHANCE = 5;

// The chest a killed monster on level `level` leaves, if any: a boss's amulet (RollBossAmulet), else ringPercent
// (MonsterKind::ringDropPercent) a ring (RollRing), else a copy of a weapon the player already has (`owned`, by
// itemIndex), never a new one, so the found weapons keep their place in the campaign.
std::optional<ItemKind> RollKillDrop(bool boss, const OwnedWeapons& owned, int level, Rng& rng, int ringPercent = 0);

// Trapped chests (plan trapped-chests): a TrappedTreasure's opening sets off one of these.
enum class ChestTrap : std::uint8_t {
	Darts, // poison darts out of the lock: CHEST_DART_DAMAGE (the trap ward cuts it) and medium poison
	Gas, // a puff of poison gas over the chest for CHEST_GAS_MS: weak poison while the player breathes it
	Curse, // an execration text on the lid: a lesser curse (RollChestCurse)
	Swarm, // CHEST_SWARM_SIZE scarabs dig out round the chest
};
constexpr int CHEST_TRAP_COUNT = 4;
// The share of each trap in percent by decoration tier (decorTier of the level): darts and gas early, the curses and
// the swarm deep.
constexpr int CHEST_TRAP_PERCENT[DECOR_TIER_COUNT][CHEST_TRAP_COUNT] = {
	{60, 40, 0, 0}, // cave
	{50, 35, 15, 0}, // worked tunnel
	{35, 30, 25, 10}, // tomb
	{30, 25, 25, 20}, // temple / necropolis
};
// Percent of the trapped chests that hold their item, with one bonus more (RollTrappedChestLoot); the others are empty.
constexpr int TRAPPED_CHEST_FULL_PERCENT = 50;
// Each a chance on the gameplay stream when the chest opens, in this order: the contents (TRAPPED_CHEST_FULL_PERCENT),
// the trap ward (AmuletBonus::trapCutPercent: the trap does not go off), the trap (RollChestTrap), then the trap's own.
// Canopic jars and pottery jars always curse, with disease; the other containers by depth.
ChestTrap RollChestTrap(Container kind, int level, Rng& rng);
// A chest's curse: disease from canopic jars and pottery jars, else weakness, clumsiness, vulnerability, fatigue or
// slowness at even odds.
Curse RollChestCurse(Container kind, Rng& rng);
// A full trapped chest: RollChestLoot, then one bonus more, as the cursed mimic's chest without its ring
// (RollCursedMimicLoot).
std::vector<ItemKind> RollTrappedChestLoot(ItemKind placed, const OwnedWeapons& owned, int level, Rng& rng);

#endif
