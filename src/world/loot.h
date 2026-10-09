#ifndef LOOT_H
#define LOOT_H

#include "items.h"
#include "rng.h"
#include <array>
#include <optional>
#include <vector>

// Contents of one treasure chest on campaign level `level`: the item placed in the map first, then random bonus items,
// from `rng` (the game's gameplay stream, which a scenario seeds). A weapon chest's bonus weapon is a weaker one the
// player holds (`owned`). Any chest but an amulet's may hold a lesser or minor amulet (RollChestAmulet).
std::vector<ItemKind> RollChestLoot(ItemKind placed, const OwnedWeapons& owned, int level, Rng& rng);

// Percent of the chests that hold an extra amulet; minor ones only from MINOR_AMULET_LEVEL on, then MINOR_AMULET_CHANCE
// percent of them.
constexpr int CHEST_AMULET_CHANCE = 10;
constexpr int MINOR_AMULET_LEVEL = 11;
constexpr int MINOR_AMULET_CHANCE = 40;
// A chest's extra amulet, if the roll gives one: lesser or minor, any type that has those tiers.
std::optional<ItemKind> RollChestAmulet(int level, Rng& rng);

// A boss's amulet: normal or grand (grand at BOSS_GRAND_PERCENT_PER_LEVEL per level, 10% on level 5), any type.
constexpr int BOSS_GRAND_PERCENT_PER_LEVEL = 2;
ItemKind RollBossAmulet(int level, Rng& rng);

// The item in the chest a killed mimic leaves: a weapon the player holds or any potion but the antidote, even odds for
// weapon or potion. The club, if nothing is held.
ItemKind RollMimicLoot(const OwnedWeapons& owned, Rng& rng);

// Percent of the kills (minions and mimics excepted) that leave a weapon chest; a boss always leaves an amulet chest.
constexpr int KILL_DROP_CHANCE = 5;

// The chest a killed monster on level `level` leaves, if any: a boss's amulet (RollBossAmulet), else a copy of a
// weapon the player already has (`owned`, by itemIndex), never a new one, so the found weapons keep their place in
// the campaign.
std::optional<ItemKind> RollKillDrop(bool boss, const OwnedWeapons& owned, int level, Rng& rng);

#endif
