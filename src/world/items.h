#ifndef ITEMS_H
#define ITEMS_H

// The items the player can carry, without rendering: shared by the game, the editor, levelgen and the unit tests.

#include <array>
#include <cstdint>
#include <optional>

// Every item, in inventory screen and save order: the melee weapons, the ranged ones, then the potions.
enum class ItemKind : std::uint8_t {
	Club,
	Dagger,
	ShortSword,
	Khopesh,
	EpsilonAxe,
	DuckbillAxe,
	Mace,
	Spear,
	SelfBow, // the first ranged weapon
	CompositeBow,
	Sling,
	ThrowingStick,
	Javelin,
	SmallHealth,
	LargeHealth,
	Might,
	Armor,
	Life,
	SmallStamina,
	LargeStamina,
	Antidote,
};
constexpr int ITEM_KIND_COUNT = 21;
constexpr int WEAPON_KIND_COUNT = 13;
constexpr int POTION_KIND_COUNT = ITEM_KIND_COUNT - WEAPON_KIND_COUNT;

// The weapons the player holds, by itemIndex.
using OwnedWeapons = std::array<bool, WEAPON_KIND_COUNT>;

constexpr int itemIndex(ItemKind kind) { return static_cast<int>(kind); }
constexpr ItemKind itemAt(int index) { return static_cast<ItemKind>(index); } // 0 .. ITEM_KIND_COUNT - 1
constexpr bool isPotion(ItemKind kind) { return itemIndex(kind) >= WEAPON_KIND_COUNT; }
constexpr bool isRanged(ItemKind kind) { return itemIndex(kind) >= itemIndex(ItemKind::SelfBow) && !isPotion(kind); }

// What a ranged weapon shoots (Dungeon::Shoot): the bows an arrow, the sling a stone; the throwing stick and the
// javelin fly themselves. Endless, like the arrows: no ammunition.
enum class MissileKind : std::uint8_t { Arrow, Stone, Stick, Javelin };
constexpr int MISSILE_KIND_COUNT = 4;
constexpr MissileKind missileOf(ItemKind weapon) {
	switch (weapon) {
	case ItemKind::Sling:
		return MissileKind::Stone;
	case ItemKind::ThrowingStick:
		return MissileKind::Stick;
	case ItemKind::Javelin:
		return MissileKind::Javelin;
	default:
		return MissileKind::Arrow;
	}
}
// Thrown: the weapon itself leaves the hand (Dungeon::Shoot), so the hand is empty until the swing ends.
constexpr bool isThrown(ItemKind weapon) { return weapon == ItemKind::ThrowingStick || weapon == ItemKind::Javelin; }

// The inventory's tabs, one per group of items. Amulets and rings have no items yet.
enum class ItemGroup : std::uint8_t { Weapons, Potions, Amulets, Rings };
constexpr int ITEM_GROUP_COUNT = 4;

constexpr ItemGroup itemGroup(ItemKind kind) { return isPotion(kind) ? ItemGroup::Potions : ItemGroup::Weapons; }

// A group's items are next to each other in ItemKind order: indexes first .. first + count - 1.
struct ItemRange {
	int first;
	int count;
};
[[nodiscard]] ItemRange groupItems(ItemGroup group);

// Level files (a treasure tile's attr and value), save games and scenario scripts name an item by a type and an id.
// The weapon ids restart per type: the club and the self-bow are both 0. A new weapon takes the next free id of its
// type, so old levels and saves keep reading.
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
