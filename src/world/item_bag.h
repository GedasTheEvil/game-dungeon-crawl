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

// Amulet upgrades (docs/plan/amulet-upgrades.md): an amulet is worth lesser 1, minor 2, normal 4, grand 8 points.
// Going up a tier costs the amulet's own worth again, paid by spares of its type of its tier or lower.
[[nodiscard]] int amuletWorth(AmuletTier tier);
[[nodiscard]] std::optional<ItemKind> nextTier(ItemKind amulet); // nullopt: grand, the top

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
	int stamps[ITEM_KIND_COUNT] = {}; // when it was last added, counting up from 1; 0: never (or a save before INV6)
	int lastStamp = 0;
	ItemKind equipped = ItemKind::Club;
	std::optional<ItemKind> worn; // the amulet on, one of those counted

  public:
	ItemBag() { Reset(); }
	void Reset(); // a new game: only the club, in hand
	void Add(ItemKind kind) {
		counts[itemIndex(kind)]++;
		found[itemIndex(kind)] = true;
		stamps[itemIndex(kind)] = ++lastStamp;
	}
	// Found in a chest: added; the journal's note of its potion group, or the weapons note and its damage type's (an
	// amulet: none yet).
	void Find(ItemKind kind, Journal& journal);
	[[nodiscard]] int Count(ItemKind kind) const { return counts[itemIndex(kind)]; }
	[[nodiscard]] int Level(ItemKind kind) const { return levels[itemIndex(kind)]; }
	[[nodiscard]] bool Found(ItemKind kind) const { return found[itemIndex(kind)]; }
	[[nodiscard]] int Stamp(ItemKind kind) const { return stamps[itemIndex(kind)]; }
	[[nodiscard]] bool AnyFound(ItemGroup group) const;
	[[nodiscard]] ItemKind Equipped() const { return equipped; }
	[[nodiscard]] std::optional<ItemKind> Worn() const { return worn; }
	[[nodiscard]] OwnedWeapons Owned() const; // the weapons held (count > 0)

	[[nodiscard]] UseBlock Block(ItemKind kind, const Vitals& player) const;
	// Equips a weapon, takes a potion out of the bag (the caller applies its potionGain), or puts an amulet on (takes
	// it off if it is the one worn; the caller applies amuletBonus(Worn())). False if Block says no.
	bool Use(ItemKind kind, const Vitals& player);
	// A weapon: its copies reach upgradeCost. An amulet: UpgradePoints reach its worth (amuletWorth), below grand.
	[[nodiscard]] bool CanUpgrade(ItemKind kind, bool alive) const;
	// False if CanUpgrade says no. An amulet becomes one of the next tier (nextTier), the spares paying largest first;
	// the worn one stays worn (the caller applies amuletBonus(Worn())).
	bool Upgrade(ItemKind kind, bool alive);
	// The points the spares of an amulet's type could pay to upgrade it: the ones of its tier or lower, but the
	// amulet itself and the worn one.
	[[nodiscard]] int UpgradePoints(ItemKind amulet) const;
	// The potion the quick-drink key would take (quickPotion), counting from `current` of `max`.
	[[nodiscard]] std::optional<ItemKind> QuickChoice(QuickKind kind, int current, int max) const;

	// "INV6 <slots> <counts...> <levels...> <found...> <stamps...> <equipped type> <equipped id> <worn type> <worn id>"
	// (worn type 0: none). Load also reads INV5 (no stamps), INV4 (no resistance potions: the amulets right after the
	// antidote), INV3 (no worn amulet), INV2 (no found flags: found is what is held) and the older saves, which start
	// straight with the 9 counts of the original slots.
	void Save(std::ostream& out) const;
	void Load(std::istream& in);
};

// How a tab sorts its found items (docs/plan/inventory-sort-orders.md). Found: ItemKind order. Name: A to Z. Strength:
// strongest first (weapons by damage at their level, amulets by tier, the large and greater potions before the
// rest), ties in ItemKind order. Recent: the last added first, the ones never stamped after them in ItemKind order.
enum class SortOrder : std::uint8_t { Found, Name, Strength, Recent };
constexpr int SORT_ORDER_COUNT = 4;

// A tab's items in the order the inventory shows them (docs/plan/solved/inventory-sorting.md): the ones found first,
// in `sort` order, then the ones not found yet in ItemKind order, so the found ones fill the first rows without gaps.
[[nodiscard]] std::vector<ItemKind> tabOrder(const ItemBag& bag, ItemGroup group, SortOrder sort = SortOrder::Found);
// kind's place in its tab's tabOrder.
[[nodiscard]] int tabPosition(const ItemBag& bag, ItemKind kind, SortOrder sort = SortOrder::Found);

#endif
