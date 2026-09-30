#ifndef LOOT_H
#define LOOT_H

#include "items.h"
#include "rng.h"
#include <vector>

// Contents of one treasure chest: the item placed in the map first, then random bonus items, from `rng` (the game's
// gameplay stream, which a scenario seeds).
std::vector<ItemKind> RollChestLoot(ItemKind placed, Rng& rng);

// The item in the chest a killed mimic leaves: any weapon or potion, even odds for weapon or potion.
ItemKind RollMimicLoot(Rng& rng);

#endif
