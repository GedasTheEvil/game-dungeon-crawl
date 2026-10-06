#ifndef LOOT_H
#define LOOT_H

#include "items.h"
#include "rng.h"
#include <array>
#include <optional>
#include <vector>

// Contents of one treasure chest: the item placed in the map first, then random bonus items, from `rng` (the game's
// gameplay stream, which a scenario seeds).
std::vector<ItemKind> RollChestLoot(ItemKind placed, Rng& rng);

// The item in the chest a killed mimic leaves: any weapon or potion but the antidote, even odds for weapon or potion.
ItemKind RollMimicLoot(Rng& rng);

// Percent of the kills (minions and mimics excepted) that leave a weapon chest; a boss always leaves one.
constexpr int KILL_DROP_CHANCE = 5;

// The chest a killed monster leaves, if any: a copy of a weapon the player already has (`owned`, by itemIndex),
// never a new one, so the found weapons keep their place in the campaign.
std::optional<ItemKind> RollKillDrop(bool boss, const std::array<bool, WEAPON_KIND_COUNT>& owned, Rng& rng);

#endif
