#include "items.h"
#include <array>

namespace {
constexpr std::array<ItemFileId, ITEM_KIND_COUNT> FILE_IDS = {{
	{ItemType::MELEE_WEAPON, 0},
	{ItemType::MELEE_WEAPON, 1},
	{ItemType::MELEE_WEAPON, 2},
	{ItemType::RANGED_WEAPON, 0},
	{ItemType::POTION, 0},
	{ItemType::POTION, 1},
	{ItemType::POTION, 2},
	{ItemType::POTION, 3},
	{ItemType::POTION, 4},
	{ItemType::POTION, 5},
	{ItemType::POTION, 6},
}};

constexpr std::array<ItemText, ITEM_KIND_COUNT> TEXTS = {{
	{"Club", "Club", "club", "", "Good old club.", "Now with spikes."},
	{"Sword", "Sword", "sword", "", "Bronze blade of a", "forgotten guard."},
	{"Spear", "Spear", "spear", "", "Long reach.", "None shall pass!"},
	{"Bow", "Bow", "bow", "", "The simple bow.", "For slow monsters."},
	{"Small Health", "Heal", "small health", "Heals 25% of max health", "Bitter herbs from", "the Nile marshes."},
	{"Large Health", "Heal+", "large health", "Heals 50% of max health", "Brewed by the priests", "of Sekhmet."},
	{"Aphethamine", "Might", "might", "Might +2, for good", "It tingles. Best not", "ask what is in it."},
	{"Stone Skin", "Armor", "armor", "Armor +2, for good", "Skin as hard as", "temple granite."},
	{"Elixir of Life", "Life", "life", "Max health +5%, full heal", "The breath of Osiris,", "sealed in a flask."},
	{"Small Stamina", "Vigor", "small stamina", "Restores 50% of stamina", "Date wine and honey.", "Mostly honey."},
	{"Large Stamina", "Vigor+", "large stamina", "Restores all stamina", "Sun-steeped water", "from the temple of Ra."},
}};
} // namespace

ItemFileId fileIdOf(ItemKind kind) { return FILE_IDS[static_cast<size_t>(itemIndex(kind))]; }

std::optional<ItemKind> itemFromFile(int type, int id) {
	for (int i = 0; i < ITEM_KIND_COUNT; i++)
		if (FILE_IDS[static_cast<size_t>(i)].type == type && FILE_IDS[static_cast<size_t>(i)].id == id)
			return itemAt(i);
	return std::nullopt;
}

const ItemText& itemText(ItemKind kind) { return TEXTS[static_cast<size_t>(itemIndex(kind))]; }
