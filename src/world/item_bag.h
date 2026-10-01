#ifndef ITEM_BAG_H
#define ITEM_BAG_H

// What the player carries and the rules for using it, without the screen (ui/inventory.h draws it) or the player:
// the caller passes the player's Vitals and applies a potion's PotionGain.

#include "items.h"
#include "quick_potion.h"
#include <cstdint>
#include <iosfwd>
#include <optional>

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
};

// Why an item cannot be used (equipped or drunk) now.
enum class UseBlock : std::uint8_t { None, Dead, NotFound, NoneLeft, Equipped, HealthFull, StaminaFull };

// What drinking a potion does to the player. Percentages of max health / max stamina; maxHpPercent grows max health,
// then heals fully.
struct PotionGain {
	int healPercent = 0;
	int staminaPercent = 0;
	int might = 0;
	int armor = 0;
	int maxHpPercent = 0;
};
[[nodiscard]] PotionGain potionGain(ItemKind potion);

class ItemBag {
  private:
	int counts[ITEM_KIND_COUNT] = {};
	int levels[ITEM_KIND_COUNT] = {}; // weapons only, from 1
	ItemKind equipped = ItemKind::Club;

  public:
	ItemBag() { Reset(); }
	void Reset(); // a new game: only the club, in hand
	void Add(ItemKind kind) { counts[itemIndex(kind)]++; }
	void Find(ItemKind kind, Journal& journal); // found in a chest: added, a weapon writes the journal's weapons note
	[[nodiscard]] int Count(ItemKind kind) const { return counts[itemIndex(kind)]; }
	[[nodiscard]] int Level(ItemKind kind) const { return levels[itemIndex(kind)]; }
	[[nodiscard]] ItemKind Equipped() const { return equipped; }

	[[nodiscard]] UseBlock Block(ItemKind kind, const Vitals& player) const;
	// Equips a weapon or takes a potion out of the bag (the caller applies its potionGain). False if Block says no.
	bool Use(ItemKind kind, const Vitals& player);
	[[nodiscard]] bool CanUpgrade(ItemKind kind, bool alive) const;
	bool Upgrade(ItemKind kind, bool alive); // false if CanUpgrade says no
	// The potion the quick-drink key would take (quickPotion), counting from `current` of `max`.
	[[nodiscard]] std::optional<ItemKind> QuickChoice(QuickKind kind, int current, int max) const;

	// "INV2 <slots> <counts...> <levels...> <equipped type> <equipped id>". Load also reads the older saves, which
	// start straight with the 9 counts of the original slots.
	void Save(std::ostream& out) const;
	void Load(std::istream& in);
};

#endif
