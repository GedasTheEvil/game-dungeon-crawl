#include "items.h"
#include <array>

namespace {
constexpr std::array<ItemFileId, ITEM_KIND_COUNT> FILE_IDS = {{
	{ItemType::MELEE_WEAPON, 0},  {ItemType::MELEE_WEAPON, 3},	{ItemType::MELEE_WEAPON, 1},
	{ItemType::MELEE_WEAPON, 4},  {ItemType::MELEE_WEAPON, 5},	{ItemType::MELEE_WEAPON, 6},
	{ItemType::MELEE_WEAPON, 7},  {ItemType::MELEE_WEAPON, 2},	{ItemType::RANGED_WEAPON, 0},
	{ItemType::RANGED_WEAPON, 1}, {ItemType::RANGED_WEAPON, 2}, {ItemType::RANGED_WEAPON, 3},
	{ItemType::RANGED_WEAPON, 4}, {ItemType::POTION, 0},		{ItemType::POTION, 1},
	{ItemType::POTION, 2},		  {ItemType::POTION, 3},		{ItemType::POTION, 4},
	{ItemType::POTION, 5},		  {ItemType::POTION, 6},		{ItemType::POTION, 7},
}};

constexpr std::array<ItemText, ITEM_KIND_COUNT> TEXTS = {{
	{"Club", "Club", "club", "", "Good old club.", "Now with spikes."},
	{"Dagger", "Dagger", "dagger", "", "Quick and close.", "Mind the fingers."},
	{"Short Sword", "Sword", "short sword", "", "Bronze blade of a", "forgotten guard."},
	{"Khopesh", "Khopesh", "khopesh", "", "The sickle sword", "of the pharaoh's guard."},
	{"Epsilon Axe", "Ep. axe", "epsilon axe", "", "A broad bronze crescent.", "Swing it and wait."},
	{"Duckbill Axe", "Db. axe", "duckbill axe", "", "Narrow and heavy.", "It goes through shells."},
	{"Mace", "Mace", "mace", "", "A stone on a stick.", "The oldest argument."},
	{"Spear", "Spear", "spear", "", "Long reach.", "None shall pass!"},
	{"Self-Bow", "Bow", "self-bow", "", "One stave of acacia.", "For slow monsters."},
	{"Composite Bow", "Comp. bow", "composite bow", "", "Wood, horn and sinew,", "glued by a master."},
	{"Sling", "Sling", "sling", "", "A shepherd's sling.", "Stones are everywhere."},
	{"Throwing Stick", "Stick", "throwing stick", "", "Brings down a duck.", "Comes back, mostly."},
	{"Javelin", "Javelin", "javelin", "", "Light enough to throw,", "heavy enough to stay."},
	{"Small Health", "Heal", "small health", "Heals 25% of max health", "Bitter herbs from", "the Nile marshes."},
	{"Large Health", "Heal+", "large health", "Heals 50% of max health", "Brewed by the priests", "of Sekhmet."},
	{"Aphethamine", "Might", "might", "Might +2, for good", "It tingles. Best not", "ask what is in it."},
	{"Stone Skin", "Armor", "armor", "Armor +2, for good", "Skin as hard as", "temple granite."},
	{"Elixir of Life", "Life", "life", "Max health +5%, full heal", "The breath of Osiris,", "sealed in a flask."},
	{"Small Stamina", "Vigor", "small stamina", "Restores 50% of stamina", "Date wine and honey.", "Mostly honey."},
	{"Large Stamina", "Vigor+", "large stamina", "Restores all stamina", "Sun-steeped water", "from the temple of Ra."},
	{"Antidote", "Cure", "antidote", "Cures all poison", "Milk of the snake", "goddess Renenutet."},
}};
} // namespace

ItemFileId fileIdOf(ItemKind kind) { return FILE_IDS[static_cast<size_t>(itemIndex(kind))]; }

std::optional<ItemKind> itemFromFile(int type, int id) {
	for (int i = 0; i < ITEM_KIND_COUNT; i++)
		if (FILE_IDS[static_cast<size_t>(i)].type == type && FILE_IDS[static_cast<size_t>(i)].id == id)
			return itemAt(i);
	return std::nullopt;
}

ItemRange groupItems(ItemGroup group) {
	ItemRange range = {ITEM_KIND_COUNT, 0};
	for (int i = ITEM_KIND_COUNT - 1; i >= 0; i--) {
		if (itemGroup(itemAt(i)) == group) {
			range.first = i;
			range.count++;
		}
	}
	return range;
}

const ItemText& itemText(ItemKind kind) { return TEXTS[static_cast<size_t>(itemIndex(kind))]; }
