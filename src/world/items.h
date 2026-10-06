#ifndef ITEMS_H
#define ITEMS_H

// The items the player can carry, without rendering: shared by the game, the editor, levelgen and the unit tests.

#include <cstdint>
#include <optional>

// Every item, in inventory screen and save order: the weapons, then the potions.
enum class ItemKind : std::uint8_t {
	Club,
	Sword,
	Spear,
	Bow,
	SmallHealth,
	LargeHealth,
	Might,
	Armor,
	Life,
	SmallStamina,
	LargeStamina,
	Antidote,
};
constexpr int ITEM_KIND_COUNT = 12;
constexpr int WEAPON_KIND_COUNT = 4;
constexpr int POTION_KIND_COUNT = ITEM_KIND_COUNT - WEAPON_KIND_COUNT;

constexpr int itemIndex(ItemKind kind) { return static_cast<int>(kind); }
constexpr ItemKind itemAt(int index) { return static_cast<ItemKind>(index); } // 0 .. ITEM_KIND_COUNT - 1
constexpr bool isPotion(ItemKind kind) { return itemIndex(kind) >= WEAPON_KIND_COUNT; }
constexpr bool isRanged(ItemKind kind) { return kind == ItemKind::Bow; }

// Level files (a treasure tile's attr and value), save games and scenario scripts name an item by a type and an id.
// The weapon ids restart per type: the club and the bow are both 0.
namespace ItemType {
constexpr int EMPTY = 0; // an empty treasure chest
constexpr int MELEE_WEAPON = 1;
constexpr int RANGED_WEAPON = 2;
constexpr int POTION = 3;
} // namespace ItemType

struct ItemFileId {
	int type; // ItemType
	int id;
};

[[nodiscard]] ItemFileId fileIdOf(ItemKind kind);
// nullopt for an unknown pair (and for an empty chest).
[[nodiscard]] std::optional<ItemKind> itemFromFile(int type, int id);

struct ItemText {
	const char* name;
	const char* shortName; // fits an inventory slot's name band
	const char* label;	   // plain word for the editor ("small health", "might")
	const char* effect;	   // potions only
	const char* lore1;
	const char* lore2;
};
[[nodiscard]] const ItemText& itemText(ItemKind kind);

// Potion strengths, in percent of the player's max health or max stamina.
namespace PotionEffect {
constexpr int SMALL_HEAL_PERCENT = 25;
constexpr int LARGE_HEAL_PERCENT = 50;
constexpr int SMALL_STAMINA_PERCENT = 50;
constexpr int LARGE_STAMINA_PERCENT = 100;
constexpr int MIGHT = 2;			   // might potion, for good
constexpr int ARMOR = 2;			   // armour potion, for good
constexpr int LIFE_MAX_HP_PERCENT = 5; // elixir of life: more max health, then a full heal
} // namespace PotionEffect

#endif
