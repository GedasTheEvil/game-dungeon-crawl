#ifndef ITEM_BAG_H
#define ITEM_BAG_H

// What the player carries and the rules for using it, without the screen (ui/inventory.h draws it) or the player:
// the caller passes the player's Vitals and applies a potion's PotionGain.

#include "items.h"
#include "quick_potion.h"
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <vector>

class Journal;

// Weapon levels: going from level L to L + 1 takes 1 + L(L + 1)/2 copies collected (2, 4, 7, 11). Each level adds
// the weapon's growth share of its base damage: the club grows most, the sword least (it starts strongest).
constexpr int MAX_WEAPON_LEVEL = 5;
[[nodiscard]] int upgradeCost(int level); // copies needed to go from `level` to the next one
[[nodiscard]] int weaponGrowthPercent(ItemKind weapon);
[[nodiscard]] int weaponDamage(ItemKind weapon, int baseDamage, int level);

// The player as the rules see it.
struct Vitals {
	bool alive = true;
	int hp = 0, maxHp = 0;
	int stamina = 0, maxStamina = 0;
	bool poisoned = false;
	int resistPercent = 0; // a resistance potion's still running, 0: none
};

// Why an item cannot be used (equipped or drunk) now.
enum class UseBlock : std::uint8_t {
	None,
	Dead,
	NotFound,
	NoneLeft,
	Equipped,
	HealthFull,
	StaminaFull,
	NotPoisoned,
	StrongerResistance
};

class ItemBag {
  private:
	int counts[ITEM_KIND_COUNT] = {};
	int levels[ITEM_KIND_COUNT] = {}; // weapons only, from 1
	bool found[ITEM_KIND_COUNT] = {}; // ever had one, also when all are used up
	ItemKind equipped = ItemKind::Club;
	std::optional<ItemKind> worn; // the amulet on, one of those counted

  public:
	ItemBag() { Reset(); }
	void Reset(); // a new game: only the club, in hand
	void Add(ItemKind kind) {
		counts[itemIndex(kind)]++;
		found[itemIndex(kind)] = true;
	}
	// Found in a chest: added; the journal's note of its potion group, or the weapons note and its damage type's (an
	// amulet: none yet).
	void Find(ItemKind kind, Journal& journal);
	[[nodiscard]] int Count(ItemKind kind) const { return counts[itemIndex(kind)]; }
	[[nodiscard]] int Level(ItemKind kind) const { return levels[itemIndex(kind)]; }
	[[nodiscard]] bool Found(ItemKind kind) const { return found[itemIndex(kind)]; }
	[[nodiscard]] bool AnyFound(ItemGroup group) const;
	[[nodiscard]] ItemKind Equipped() const { return equipped; }
	[[nodiscard]] std::optional<ItemKind> Worn() const { return worn; }
	[[nodiscard]] OwnedWeapons Owned() const; // the weapons held (count > 0)

	[[nodiscard]] UseBlock Block(ItemKind kind, const Vitals& player) const;
	// Equips a weapon, takes a potion out of the bag (the caller applies its potionGain), or puts an amulet on (takes
	// it off if it is the one worn; the caller applies amuletBonus(Worn())). False if Block says no.
	bool Use(ItemKind kind, const Vitals& player);
	[[nodiscard]] bool CanUpgrade(ItemKind kind, bool alive) const;
	bool Upgrade(ItemKind kind, bool alive); // false if CanUpgrade says no
	// The potion the quick-drink key would take (quickPotion), counting from `current` of `max`.
	[[nodiscard]] std::optional<ItemKind> QuickChoice(QuickKind kind, int current, int max) const;

	// "INV5 <slots> <counts...> <levels...> <found...> <equipped type> <equipped id> <worn type> <worn id>" (worn type
	// 0: none). Load also reads INV4 (no resistance potions: the amulets right after the antidote), INV3 (no worn
	// amulet), INV2 (no found flags: found is what is held) and the older saves, which start straight with the 9 counts
	// of the original slots.
	void Save(std::ostream& out) const;
	void Load(std::istream& in);
};

// A tab's items in the order the inventory shows them (docs/plan/solved/inventory-sorting.md): the ones found first,
// then the ones not found yet, each in ItemKind order, so the found ones fill the first rows without gaps.
[[nodiscard]] std::vector<ItemKind> tabOrder(const ItemBag& bag, ItemGroup group);
// kind's place in its tab's tabOrder.
[[nodiscard]] int tabPosition(const ItemBag& bag, ItemKind kind);

#endif
