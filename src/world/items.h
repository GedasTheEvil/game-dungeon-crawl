#ifndef ITEMS_H
#define ITEMS_H

// The items the player can carry, without rendering: shared by the game, the editor, levelgen and the unit tests.

#include "damage.h"
#include <array>
#include <cstdint>
#include <optional>

// Every item, in inventory screen and save order: the melee weapons, the ranged ones, the potions, then the amulets.
// New kinds go at the end of their group, and groups only at the end, so a save's slots stay in this order.
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
	// The amulets (docs/plan/amulets.draft.md), one inventory row per type: lesser, minor, normal, grand.
	StrengthLesser, // the first amulet
	StrengthMinor,
	StrengthNormal,
	StrengthGrand,
	ArmorLesser,
	ArmorMinor,
	ArmorNormal,
	ArmorGrand,
	HealthLesser,
	HealthMinor,
	HealthNormal,
	HealthGrand,
	PoisonWardLesser,
	PoisonWardMinor,
	PoisonWardNormal,
	PoisonWardGrand,
	TrapWardLesser,
	TrapWardMinor,
	TrapWardNormal,
	TrapWardGrand,
	BluntWardLesser,
	BluntWardMinor,
	BluntWardNormal,
	BluntWardGrand,
	SlashWardLesser,
	SlashWardMinor,
	SlashWardNormal,
	SlashWardGrand,
	PierceWardLesser,
	PierceWardMinor,
	PierceWardNormal,
	PierceWardGrand,
	RegenerationNormal, // regeneration has no lesser or minor tier
	RegenerationGrand,
};
constexpr int ITEM_KIND_COUNT = static_cast<int>(ItemKind::RegenerationGrand) + 1;
constexpr int WEAPON_KIND_COUNT = 13;
constexpr int FIRST_AMULET = static_cast<int>(ItemKind::StrengthLesser);
constexpr int POTION_KIND_COUNT = FIRST_AMULET - WEAPON_KIND_COUNT;
constexpr int AMULET_KIND_COUNT = ITEM_KIND_COUNT - FIRST_AMULET;
// Saves from the Egyptian weapons on have their slots in ItemKind order: this many, or more since the amulets.
constexpr int ORDERED_SAVE_SLOTS = FIRST_AMULET;

// The weapons the player holds, by itemIndex.
using OwnedWeapons = std::array<bool, WEAPON_KIND_COUNT>;

constexpr int itemIndex(ItemKind kind) { return static_cast<int>(kind); }
constexpr ItemKind itemAt(int index) { return static_cast<ItemKind>(index); } // 0 .. ITEM_KIND_COUNT - 1
constexpr bool isWeapon(ItemKind kind) { return itemIndex(kind) < WEAPON_KIND_COUNT; }
constexpr bool isPotion(ItemKind kind) {
	return itemIndex(kind) >= WEAPON_KIND_COUNT && itemIndex(kind) < FIRST_AMULET;
}
constexpr bool isAmulet(ItemKind kind) { return itemIndex(kind) >= FIRST_AMULET; }
constexpr bool isRanged(ItemKind kind) { return itemIndex(kind) >= itemIndex(ItemKind::SelfBow) && isWeapon(kind); }

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

// The inventory's tabs, one per group of items. Rings have no items yet.
enum class ItemGroup : std::uint8_t { Weapons, Potions, Amulets, Rings };
constexpr int ITEM_GROUP_COUNT = 4;

constexpr ItemGroup itemGroup(ItemKind kind) {
	return isAmulet(kind) ? ItemGroup::Amulets : isPotion(kind) ? ItemGroup::Potions : ItemGroup::Weapons;
}

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
constexpr int AMULET = 4; // id: the amulet's place among the amulets (0 = lesser strength)
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
	const char* effect;	   // potions and amulets
	const char* lore1;
	const char* lore2;
};
[[nodiscard]] const ItemText& itemText(ItemKind kind);
// A weapon's damage mix: blunt, slash, pierce percent. Its main type (mainType) is the journal's.
[[nodiscard]] const DamageMix& weaponMix(ItemKind weapon);

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

// One amulet is worn at a time; it gives its bonus while it is on. All tiers of a type share the model.
enum class AmuletType : std::uint8_t {
	Strength,
	Armor,
	Health,
	PoisonWard,
	TrapWard,
	BluntWard,
	SlashWard,
	PierceWard,
	Regeneration,
};
constexpr int AMULET_TYPE_COUNT = 9;
enum class AmuletTier : std::uint8_t { Lesser, Minor, Normal, Grand };
constexpr int AMULET_TIER_COUNT = 4;

struct Amulet {
	AmuletType type;
	AmuletTier tier;
};
[[nodiscard]] Amulet amuletOf(ItemKind amulet);
// nullopt for a tier the type does not have (lesser and minor regeneration).
[[nodiscard]] std::optional<ItemKind> amuletKind(AmuletType type, AmuletTier tier);

// What the worn amulet gives, all zero without one.
struct AmuletBonus {
	int might = 0;				 // added to every hit
	int armor = 0;				 // taken off every hit, like the armour
	int maxHpPercent = 0;		 // more max HP, a share of the base max HP
	int poisonResistPercent = 0; // chance that a poisoned hit does not poison
	int trapCutPercent = 0;		 // less spike and death trap damage; 100: immune
	int regenHpPerSecond = 0;	 // while no monster chases the player and no poison runs
	Resistances resist = NO_RESISTANCES;
};
[[nodiscard]] AmuletBonus amuletBonus(std::optional<ItemKind> worn);

#endif
