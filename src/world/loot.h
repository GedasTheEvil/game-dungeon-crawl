#ifndef LOOT_H
#define LOOT_H

#include <vector>

struct LootItem {
	int type; // ItemType
	int id;
};

// Contents of one treasure chest: the item placed in the map first, then random bonus items.
// Uses rand(), so scenario runs with a fixed seed get the same loot every time.
std::vector<LootItem> RollChestLoot(int type, int id);

#endif
