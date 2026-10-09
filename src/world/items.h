#ifndef ITEMS_H
#define ITEMS_H

// The items the player can carry, without rendering: shared by the game, the editor, levelgen and the unit tests.

#include "damage.h"
#include "poison.h"
#include "rgb.h"
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
	LesserResistance, // docs/plan/resistance-potion.md
	GreaterResistance,
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
	VenomLesser, // docs/plan/solved/venom-amulet.md
	VenomMinor,
	VenomNormal,
	VenomGrand,
};
constexpr int ITEM_KIND_COUNT = static_cast<int>(ItemKind::VenomGrand) + 1;
constexpr int WEAPON_KIND_COUNT = static_cast<int>(ItemKind::SmallHealth); // the first potion
constexpr int FIRST_AMULET = static_cast<int>(ItemKind::StrengthLesser);
constexpr int POTION_KIND_COUNT = FIRST_AMULET - WEAPON_KIND_COUNT;
constexpr int AMULET_KIND_COUNT = ITEM_KIND_COUNT - FIRST_AMULET;
// The weapons the player holds, by itemIndex.
using OwnedWeapons = std::array<bool, WEAPON_KIND_COUNT>;

constexpr int itemIndex(ItemKind kind) { return static_cast<int>(kind); }
constexpr ItemKind itemAt(int index) { return static_cast<ItemKind>(index); } // 0 .. ITEM_KIND_COUNT - 1
// Saves from the Egyptian weapons on have their slots in ItemKind order: this many (the weapons and the potions up to
// the antidote), or more since the amulets.
constexpr int ORDERED_SAVE_SLOTS = itemIndex(ItemKind::Antidote) + 1;
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

// How a weapon is held and swung (drawWeapon). Tilts in degrees from upright, towards the facing side. An attack
// raises the weapon back to windupTilt, brings it down through strikeTilt, where the hit lands (hitMs), and returns
// it to restTilt by swingMs. The bow is drawn until hitMs instead and the arrow leaves. hitMs is the frame delay (the
// wind-up, the draw), recoveryMs the time from the hit until the next attack can begin; swingMs runs inside the
// recovery.
struct WeaponMotion {
	float grip = 0.1f; // the fist holds it this far up its length, from the lowest point
	float restTilt = 45.f, windupTilt = 45.f, strikeTilt = 45.f;
	float thrust = 0.f; // pushed forward this many lengths at the strike instead (the spear)
	int hitMs = 200, swingMs = 400;
	int recoveryMs = 800;
	[[nodiscard]] int AttackMs() const { return hitMs + recoveryMs; } // from one attack to the next
};

// What drinking a potion does to the player. Percentages of max health / max stamina; maxHpPercent grows max health,
// then heals fully.
struct PotionGain {
	int healPercent = 0;
	int staminaPercent = 0;
	int might = 0;
	int armor = 0;
	int maxHpPercent = 0;
	bool cure = false;	   // ends all poison
	int resistPercent = 0; // for PotionEffect::RESIST_MS, the chance a poisoned hit does not poison
};

enum class FieldNote : unsigned char; // journal.h

struct ItemText {
	const char* name;
	const char* shortName; // fits an inventory slot's name band
	const char* label;	   // plain word for the editor ("small health", "might")
	const char* effect;	   // potions and amulets
	const char* lore1;
	const char* lore2;
};
[[nodiscard]] const ItemText& itemText(ItemKind kind);

struct WeaponDef {
	const char* model = ""; // models/items/<model>.md3, textures/items/<model>.png
	float scale = 1.f;
	int damage = 0;	 // at weapon level 1; the loot grades go by it (loot.cpp)
	int range = 0;	 // tenths of a tile (a ranged weapon's: how far it aims)
	DamageMix mix{}; // blunt, slash, pierce percent. Its main type (mainType) is the journal's.
	// Each weapon level adds this share of the base damage: the club grows most, the sword least (it starts
	// strongest).
	int growthPercent = 20;
	WeaponMotion motion{};
	const char* swingSound = "";			  // sounds/items/<name>.wav: the attack begins
	const char* strikeSound = "";			  // a melee hit lands, the shot leaves
	MissileKind missile = MissileKind::Arrow; // ranged weapons
	bool thrown = false;   // the weapon itself leaves the hand (Dungeon::Shoot): the hand is empty until the swing ends
	int generatedFrom = 0; // levelgen's weapon chests hold it from this difficulty on (0: from the start)
};

// The vessels the potions come in (tools/blender/models/items.py). Several potions share one, each with its own
// texture: look-alikes differ by the colour of the liquid.
enum class PotionModel : std::uint8_t { Flask, Lotus, Pilgrim, Canopic, Cobra, Ankh };
constexpr int POTION_MODEL_COUNT = 6;

struct PotionModelDef {
	const char* model; // models/items/<model>.md3
	float scale;	   // on a chest (Item::scale)
	float detailScale; // the inventory's detail view
};
[[nodiscard]] const PotionModelDef& potionModelDef(PotionModel model);

struct PotionDef {
	PotionGain gain{};
	PotionModel model{};
	const char* texture = ""; // textures/items/<texture>.png, on its model's UVs
	Rgb colour{};			  // the icons' tint: the inventory's potions tab, the HUD quick slot
	FieldNote note{};		  // the journal's, on the first one found
	int generatedWeight = 0;  // levelgen's potion chests, in percent
	bool mimicLoot = true;	  // a mimic can leave it (not the antidote: it lies only on the levels with poisoners)
};

// The weapons and the potions, one row each; an amulet's row is its type's (AmuletTypeDef in items.cpp).
struct ItemDef {
	ItemFileId file{};
	ItemText text{};
	WeaponDef weapon{}; // weapons only
	PotionDef potion{}; // potions only
};
[[nodiscard]] const WeaponDef& weaponDef(ItemKind weapon);
[[nodiscard]] const PotionDef& potionDef(ItemKind potion);
[[nodiscard]] inline const DamageMix& weaponMix(ItemKind weapon) { return weaponDef(weapon).mix; }
[[nodiscard]] inline MissileKind missileOf(ItemKind weapon) { return weaponDef(weapon).missile; }
[[nodiscard]] inline bool isThrown(ItemKind weapon) { return isWeapon(weapon) && weaponDef(weapon).thrown; }
[[nodiscard]] inline PotionGain potionGain(ItemKind potion) {
	return isPotion(potion) ? potionDef(potion).gain : PotionGain{};
}

// Potion strengths, in percent of the player's max health or max stamina.
namespace PotionEffect {
constexpr int SMALL_HEAL_PERCENT = 25;
constexpr int LARGE_HEAL_PERCENT = 50;
constexpr int SMALL_STAMINA_PERCENT = 50;
constexpr int LARGE_STAMINA_PERCENT = 100;
constexpr int MIGHT = 2;				  // might potion, for good
constexpr int ARMOR = 2;				  // armour potion, for good
constexpr int LIFE_MAX_HP_PERCENT = 5;	  // elixir of life: more max health, then a full heal
constexpr int LESSER_RESIST_PERCENT = 50; // resistance potions: added to the amulet's ward, up to 100%
constexpr int GREATER_RESIST_PERCENT = 95;
constexpr int RESIST_MS = 120000;
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
	Venom, // the player's hits poison
};
constexpr int AMULET_TYPE_COUNT = 10;
enum class AmuletTier : std::uint8_t { Lesser, Minor, Normal, Grand };
constexpr int AMULET_TIER_COUNT = 4;

struct Amulet {
	AmuletType type;
	AmuletTier tier;
};
[[nodiscard]] Amulet amuletOf(ItemKind amulet);
// nullopt for a tier the type does not have (lesser and minor regeneration).
[[nodiscard]] std::optional<ItemKind> amuletKind(AmuletType type, AmuletTier tier);
// The model all tiers of a type share: models/items/<name>.md3 (tools/blender/models/amulets.py).
[[nodiscard]] const char* amuletModel(AmuletType type);

// What the worn amulet gives, all zero without one.
struct AmuletBonus {
	int might = 0;				 // added to every hit
	int armor = 0;				 // taken off every hit, like the armour
	int maxHpPercent = 0;		 // more max HP, a share of the base max HP
	int poisonResistPercent = 0; // chance that a poisoned hit does not poison
	int trapCutPercent = 0;		 // less spike and death trap damage; 100: immune
	int regenHpPerSecond = 0;	 // while no monster chases the player and no poison runs
	int venomPercent = 0;		 // chance that a weapon hit poisons the monster (Monster::TakePoison)
	PoisonTier venomTier = PoisonTier::Weak;
	Resistances resist = NO_RESISTANCES;
};
[[nodiscard]] AmuletBonus amuletBonus(std::optional<ItemKind> worn);

#endif
