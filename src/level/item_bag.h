#ifndef ITEM_BAG_H
#define ITEM_BAG_H

// What the player carries and the rules for using it, without the screen (ui/inventory.h draws it) or the player:
// the caller passes the player's BagUser and applies a potion's PotionGain.

#include "items.h"
#include "quick_potion.h"
#include <algorithm>
#include <array>
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

// Amulet upgrades (plan amulet-upgrades): an amulet is worth lesser 1, minor 2, normal 4, grand 8 points. Going up a
// tier costs the amulet's own worth again, paid by spares of its type of its tier or lower.
[[nodiscard]] int amuletWorth(AmuletTier tier);
[[nodiscard]] std::optional<ItemKind> nextTier(ItemKind amulet); // nullopt: grand, the top

// Shrine offerings (plan shrine-offerings). Amulets and rings are magic: they upgrade (an amulet) or merge (two rings
// of a gem and metal into one of the next metal) only at a god's shrine. Weapons are plain craft: anywhere. At a
// shrine a spare (Spares) is offered for XP by its rank (offerXP).
enum class AtShrine : bool { No, Yes };
[[nodiscard]] std::optional<ItemKind> nextMetal(ItemKind ring); // nullopt: sky iron, the top
[[nodiscard]] int offerXP(ItemKind kind); // 0: no offering (a potion)

// The player as the bag's rules see it (whether a potion or an item can be used now).
struct BagUser {
	bool alive = true;
	int hp = 0, maxHp = 0;
	int stamina = 0, maxStamina = 0;
	bool poisoned = false;
	int resistPercent = 0; // a resistance potion's still running, 0: none
};

// How the shots of a ranged weapon come back (plan ranged-weaker-than-melee; PlayerStats::ShotPace): one every
// everyMs, the first delayMs after the weapon's last shot.
struct ShotPace {
	int delayMs = 0;
	int everyMs = 1;
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
	WornRings rings; // the rings on, left and right hand (plan ring-accessories), each one of those counted
	Hand olderHand = Hand::Left; // the ring worn longer, when both hands wear one
	// Per ranged weapon (plan ranged-weaker-than-melee): the shots spent and not back yet, and the time since its last
	// shot (the recovery's clock). Not saved: a load starts full.
	int shotsSpent[WEAPON_KIND_COUNT] = {};
	int sinceShotMs[WEAPON_KIND_COUNT] = {};

	[[nodiscard]] int HandsWearing(ItemKind ring) const; // 0, 1 or 2
	void PutOnHand(ItemKind ring, Hand hand);

  public:
	ItemBag() { Reset(); }
	void Reset(); // a new game: only the club, in hand
	void Add(ItemKind kind) {
		counts[itemIndex(kind)]++;
		found[itemIndex(kind)] = true;
		stamps[itemIndex(kind)] = ++lastStamp;
	}
	// Found in a chest: added; the journal's note of its potion group, or the weapons note and its damage type's, or
	// its amulet type's.
	void Find(ItemKind kind, Journal& journal);
	[[nodiscard]] int Count(ItemKind kind) const { return counts[itemIndex(kind)]; }
	[[nodiscard]] int Level(ItemKind kind) const { return levels[itemIndex(kind)]; }
	[[nodiscard]] bool Found(ItemKind kind) const { return found[itemIndex(kind)]; }
	[[nodiscard]] int Stamp(ItemKind kind) const { return stamps[itemIndex(kind)]; }
	[[nodiscard]] bool AnyFound(ItemGroup group) const;
	[[nodiscard]] ItemKind Equipped() const { return equipped; }
	[[nodiscard]] std::optional<ItemKind> Worn() const { return worn; }
	[[nodiscard]] const WornRings& Rings() const { return rings; }
	[[nodiscard]] std::optional<ItemKind> RingOn(Hand hand) const { return rings[static_cast<size_t>(hand)]; }
	[[nodiscard]] bool RingWorn(ItemKind ring) const { return HandsWearing(ring) > 0; }
	// The ring card's hand buttons: the ring on that hand comes off; if it is the one there, it comes off. A ring worn
	// on the other hand with no second copy held moves over. False if the ring is not held or the player is dead.
	bool WearOn(ItemKind ring, Hand hand, bool alive);
	[[nodiscard]] OwnedWeapons Owned() const; // the weapons held (count > 0)

	// Shots (plan ranged-weaker-than-melee). shots: the weapon's full set (PlayerStats::Shots). Spending one restarts
	// its recovery wait; RecoverShots runs the clock of every weapon at once (the ones not in hand too), each brings
	// back one shot a pace.everyMs once pace.delayMs have passed since its last shot.
	[[nodiscard]] int ShotsLeft(ItemKind weapon, int shots) const {
		return std::max(0, shots - shotsSpent[itemIndex(weapon)]);
	}
	void SpendShot(ItemKind weapon);
	void RecoverShots(int ms, const ShotPace& pace);
	// 0..1: how far the next shot of the weapon is back (0 while the wait after its last shot runs, or when it is
	// full).
	[[nodiscard]] float ShotRecovery(ItemKind weapon, const ShotPace& pace) const;
	void RefillShots(); // all shots back (a new game, a load)

	[[nodiscard]] UseBlock Block(ItemKind kind, const BagUser& player) const;
	// Equips a weapon, takes a potion out of the bag (the caller applies its potionGain), or puts an amulet on (takes
	// it off if it is the one worn; the caller applies amuletBonus(Worn())). A ring: worn, it comes off (the one put on
	// last, if both hands wear it); else on the free hand, left first, or in place of the one worn longer (the caller
	// applies ringBonus(Rings())). False if Block says no.
	bool Use(ItemKind kind, const BagUser& player);
	// A weapon: its copies reach upgradeCost. An amulet: at a shrine, UpgradePoints reach its worth (amuletWorth),
	// below grand.
	[[nodiscard]] bool CanUpgrade(ItemKind kind, bool alive, AtShrine shrine) const;
	// False if CanUpgrade says no. An amulet becomes one of the next tier (nextTier), the spares paying largest first;
	// the worn one stays worn (the caller applies amuletBonus(Worn())).
	bool Upgrade(ItemKind kind, bool alive, AtShrine shrine);
	// What could be offered (plan shrine-offerings): a weapon's copies past the ones its top level took (none below
	// MAX_WEAPON_LEVEL: they still count towards it), an amulet's or a ring's not worn. Potions: none.
	[[nodiscard]] int Spares(ItemKind kind) const;
	[[nodiscard]] bool CanOffer(ItemKind kind, bool alive, AtShrine shrine) const;
	bool Offer(ItemKind kind, bool alive, AtShrine shrine); // one spare gone; the caller gives offerXP
	// Two rings of the gem and metal, below sky iron: one of the next metal (nextMetal) instead. A hand that wore one
	// of the two wears the new ring (the caller applies ringBonus(Rings())); a second such hand comes free.
	[[nodiscard]] bool CanMerge(ItemKind ring, bool alive, AtShrine shrine) const;
	bool Merge(ItemKind ring, bool alive, AtShrine shrine);
	// The points the spares of an amulet's type could pay to upgrade it: the ones of its tier or lower, but the
	// amulet itself and the worn one.
	[[nodiscard]] int UpgradePoints(ItemKind amulet) const;
	// The potion the quick-drink key would take (quickPotion), counting from `current` of `max`.
	[[nodiscard]] std::optional<ItemKind> QuickChoice(QuickKind kind, int current, int max) const;

	// "INV8 <slots> <counts...> <levels...> <found...> <stamps...> <equipped type> <equipped id> <worn type> <worn id>
	// <left ring type> <left ring id> <right ring type> <right ring id> <older hand>" (type 0: none). Load also reads
	// INV7 (no rings), INV6 (no water blessing: the amulets right after the greater resistance
	// potion), INV5 (no stamps), INV4 (no resistance potions: the amulets right after the
	// antidote), INV3 (no worn amulet), INV2 (no found flags: found is what is held) and the older saves, which start
	// straight with the 9 counts of the original slots.
	void Save(std::ostream& out) const;
	void Load(std::istream& in);
};

// How a tab sorts its found items (plan inventory-sort-orders). Found: ItemKind order. Name: A to Z. Strength:
// strongest first (weapons by damage at their level, amulets by tier, the large and greater potions before the rest),
// ties in ItemKind order. Recent: the last added first, the ones never stamped after them in ItemKind order.
enum class SortOrder : std::uint8_t { Found, Name, Strength, Recent };
constexpr int SORT_ORDER_COUNT = 4;

// A tab's items in the order the inventory shows them (plan inventory-sorting): the ones found first, in `sort` order,
// then the ones not found yet in ItemKind order, so the found ones fill the first rows without gaps. Potions, amulets
// and rings: the found ones held first, then the empty ones (plan empty-slots-last), each in `sort` order.
[[nodiscard]] std::vector<ItemKind> tabOrder(const ItemBag& bag, ItemGroup group, SortOrder sort = SortOrder::Found);
// The same, with `held` saying which of them count as held (count > 0 in the plain one): the open inventory counts an
// item used up since it opened as still held, so it keeps its place.
using HeldItems = std::array<bool, ITEM_KIND_COUNT>;
[[nodiscard]] std::vector<ItemKind>
tabOrder(const ItemBag& bag, ItemGroup group, SortOrder sort, const HeldItems& held);
// kind's place in its tab's tabOrder.
[[nodiscard]] int tabPosition(const ItemBag& bag, ItemKind kind, SortOrder sort = SortOrder::Found);

#endif
