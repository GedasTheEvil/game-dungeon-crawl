#ifndef LOOT_H
#define LOOT_H

#include "items.h"
#include <vector>

// Contents of one treasure chest: the item placed in the map first, then random bonus items.
// Uses rand(), so scenario runs with a fixed seed get the same loot every time.
std::vector<ItemKind> RollChestLoot(ItemKind placed);

// The item in the chest a killed mimic leaves: any weapon or potion, even odds for weapon or potion.
ItemKind RollMimicLoot();

#endif
