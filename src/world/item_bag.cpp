#include "item_bag.h"
#include "journal.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace {
constexpr int LEGACY_SLOT_COUNT = 9; // saves from before the stamina potions and item levels

// The slots of the saves from before the Egyptian weapons, in their order; the older saves have the first 9 of them.
constexpr ItemKind OLD_SLOTS[] = {ItemKind::Club,		  ItemKind::ShortSword,	  ItemKind::Spear,
								  ItemKind::SelfBow,	  ItemKind::SmallHealth,  ItemKind::LargeHealth,
								  ItemKind::Might,		  ItemKind::Armor,		  ItemKind::Life,
								  ItemKind::SmallStamina, ItemKind::LargeStamina, ItemKind::Antidote};
constexpr int OLD_SLOT_COUNT = 12;

// The resistance potions came in after the amulets: before INV5 the amulets follow the antidote.
constexpr size_t RESISTANCE_POTIONS = itemIndex(ItemKind::GreaterResistance) - itemIndex(ItemKind::Antidote);

// The item a saved slot holds: a save with ORDERED_SAVE_SLOTS or more slots is in ItemKind order (newer kinds were
// added at the end, but for the resistance potions before INV5), an older one in OLD_SLOTS order.
std::optional<ItemKind> savedSlot(size_t slot, size_t slots, bool resistance) {
	if (slots >= ORDERED_SAVE_SLOTS) {
		const size_t index =
			resistance || slot < static_cast<size_t>(ORDERED_SAVE_SLOTS) ? slot : slot + RESISTANCE_POTIONS;
		return index < ITEM_KIND_COUNT ? std::optional(itemAt(static_cast<int>(index))) : std::nullopt;
	}
	if (slot < OLD_SLOT_COUNT)
		return OLD_SLOTS[slot];
	return std::nullopt;
}

bool heals(ItemKind kind) { return kind == ItemKind::SmallHealth || kind == ItemKind::LargeHealth; }
bool restoresStamina(ItemKind kind) { return kind == ItemKind::SmallStamina || kind == ItemKind::LargeStamina; }
} // namespace

int upgradeCost(int level) { return 1 + level * (level + 1) / 2; }

int weaponGrowthPercent(ItemKind weapon) { return weaponDef(weapon).growthPercent; }

int weaponDamage(ItemKind weapon, int baseDamage, int level) {
	const int percent = 100 + weaponGrowthPercent(weapon) * (level - 1);
	return static_cast<int>(std::lround(static_cast<double>(baseDamage) * percent / 100.0));
}

int amuletWorth(AmuletTier tier) { return 1 << static_cast<int>(tier); }

std::optional<ItemKind> nextTier(ItemKind amulet) {
	const Amulet a = amuletOf(amulet);
	if (a.tier == AmuletTier::Grand)
		return std::nullopt;
	return amuletKind(a.type, static_cast<AmuletTier>(static_cast<int>(a.tier) + 1));
}

void ItemBag::Reset() {
	for (int& count : counts)
		count = 0;
	for (bool& f : found)
		f = false;
	for (int& stamp : stamps)
		stamp = 0;
	lastStamp = 0;
	Add(ItemKind::Club); // everyone starts with the club
	for (int& level : levels)
		level = 1;
	equipped = ItemKind::Club;
	worn.reset();
}

OwnedWeapons ItemBag::Owned() const {
	OwnedWeapons owned{};
	for (int i = 0; i < WEAPON_KIND_COUNT; i++)
		owned[static_cast<size_t>(i)] = counts[i] > 0;
	return owned;
}

bool ItemBag::AnyFound(ItemGroup group) const {
	const ItemRange range = groupItems(group);
	for (int i = range.first; i < range.first + range.count; i++)
		if (found[i])
			return true;
	return false;
}

namespace {
// Higher: stronger, sorted first by SortOrder::Strength.
int strength(const ItemBag& bag, ItemKind kind) {
	if (isWeapon(kind))
		return weaponDamage(kind, weaponDef(kind).damage, bag.Level(kind));
	if (isAmulet(kind))
		return static_cast<int>(amuletOf(kind).tier);
	const bool strong =
		kind == ItemKind::LargeHealth || kind == ItemKind::LargeStamina || kind == ItemKind::GreaterResistance;
	return strong ? 1 : 0;
}

// a before b in `sort` order; neither before the other: ItemKind order (the sort is stable).
bool sortsBefore(const ItemBag& bag, SortOrder sort, ItemKind a, ItemKind b) {
	switch (sort) {
	case SortOrder::Found:
		return false;
	case SortOrder::Name:
		return std::strcmp(itemText(a).name, itemText(b).name) < 0;
	case SortOrder::Strength:
		return strength(bag, a) > strength(bag, b);
	case SortOrder::Recent:
		return bag.Stamp(a) > bag.Stamp(b);
	}
	return false;
}
} // namespace

std::vector<ItemKind> tabOrder(const ItemBag& bag, ItemGroup group, SortOrder sort) {
	const ItemRange range = groupItems(group);
	std::vector<ItemKind> order;
	order.reserve(static_cast<size_t>(range.count));
	for (bool found : {true, false})
		for (int i = range.first; i < range.first + range.count; i++)
			if (bag.Found(itemAt(i)) == found)
				order.push_back(itemAt(i));
	const auto foundEnd = std::find_if(order.begin(), order.end(), [&](ItemKind k) { return !bag.Found(k); });
	std::stable_sort(order.begin(), foundEnd, [&](ItemKind a, ItemKind b) { return sortsBefore(bag, sort, a, b); });
	return order;
}

int tabPosition(const ItemBag& bag, ItemKind kind, SortOrder sort) {
	const std::vector<ItemKind> order = tabOrder(bag, itemGroup(kind), sort);
	return static_cast<int>(std::find(order.begin(), order.end(), kind) - order.begin());
}

UseBlock ItemBag::Block(ItemKind kind, const Vitals& player) const {
	if (!player.alive)
		return UseBlock::Dead;
	if (Count(kind) <= 0)
		return Found(kind) ? UseBlock::NoneLeft : UseBlock::NotFound;
	if (isAmulet(kind)) // put on, or taken off when worn
		return UseBlock::None;
	if (kind == equipped)
		return UseBlock::Equipped;
	if (heals(kind) && player.hp >= player.maxHp)
		return UseBlock::HealthFull;
	if (restoresStamina(kind) && player.stamina >= player.maxStamina)
		return UseBlock::StaminaFull;
	if (kind == ItemKind::Antidote && !player.poisoned)
		return UseBlock::NotPoisoned;
	if (potionGain(kind).resistPercent > 0 && potionGain(kind).resistPercent < player.resistPercent)
		return UseBlock::StrongerResistance;
	return UseBlock::None;
}

bool ItemBag::Use(ItemKind kind, const Vitals& player) {
	if (Block(kind, player) != UseBlock::None)
		return false;
	if (isPotion(kind))
		counts[itemIndex(kind)]--;
	else if (isAmulet(kind))
		worn = worn == kind ? std::nullopt : std::optional(kind);
	else
		equipped = kind;
	return true;
}

int ItemBag::UpgradePoints(ItemKind amulet) const {
	if (!isAmulet(amulet) || Count(amulet) <= 0)
		return 0;
	const Amulet a = amuletOf(amulet);
	int points = 0;
	for (int tier = 0; tier <= static_cast<int>(a.tier); tier++) {
		const std::optional<ItemKind> spare = amuletKind(a.type, static_cast<AmuletTier>(tier));
		if (!spare)
			continue;
		const int held = Count(*spare) - (*spare == amulet || worn == *spare ? 1 : 0);
		points += std::max(0, held) * amuletWorth(static_cast<AmuletTier>(tier));
	}
	return points;
}

bool ItemBag::CanUpgrade(ItemKind kind, bool alive) const {
	if (!alive)
		return false;
	if (isAmulet(kind))
		return nextTier(kind) && Count(kind) > 0 && UpgradePoints(kind) >= amuletWorth(amuletOf(kind).tier);
	return isWeapon(kind) && Level(kind) < MAX_WEAPON_LEVEL && Count(kind) >= upgradeCost(Level(kind));
}

bool ItemBag::Upgrade(ItemKind kind, bool alive) {
	if (!CanUpgrade(kind, alive))
		return false;
	if (!isAmulet(kind)) {
		levels[itemIndex(kind)]++;
		return true;
	}
	const std::optional<ItemKind> upgraded = nextTier(kind);
	if (!upgraded)
		return false;
	// Spares largest first: with powers of two they always pay exactly.
	const Amulet a = amuletOf(kind);
	const bool wasWorn = worn == kind;
	counts[itemIndex(kind)]--;
	int owed = amuletWorth(a.tier);
	for (int tier = static_cast<int>(a.tier); tier >= 0 && owed > 0; tier--) {
		const std::optional<ItemKind> spare = amuletKind(a.type, static_cast<AmuletTier>(tier));
		if (!spare)
			continue;
		const int worth = amuletWorth(static_cast<AmuletTier>(tier));
		const int held = Count(*spare) - (*spare != kind && worn == *spare ? 1 : 0); // kind's own is taken off already
		const int taken = std::min(std::max(0, held), owed / worth);
		counts[itemIndex(*spare)] -= taken;
		owed -= taken * worth;
	}
	Add(*upgraded);
	if (wasWorn)
		worn = upgraded;
	return true;
}

std::optional<ItemKind> ItemBag::QuickChoice(QuickKind kind, int current, int max) const {
	bool health = kind == QuickKind::Health;
	int small = Count(health ? ItemKind::SmallHealth : ItemKind::SmallStamina);
	int large = Count(health ? ItemKind::LargeHealth : ItemKind::LargeStamina);
	return quickPotion(kind, current, max, small, large);
}

void ItemBag::Save(std::ostream& out) const {
	out << "INV6 " << ITEM_KIND_COUNT << " ";
	for (int count : counts)
		out << count << " ";
	for (int level : levels)
		out << level << " ";
	for (bool f : found)
		out << (f ? 1 : 0) << " ";
	for (int stamp : stamps)
		out << stamp << " ";
	ItemFileId id = fileIdOf(equipped);
	out << id.type << " " << id.id << " ";
	const ItemFileId on = worn ? fileIdOf(*worn) : ItemFileId{ItemType::EMPTY, 0};
	out << on.type << " " << on.id << "\n";
}

void ItemBag::Load(std::istream& in) {
	for (int& count : counts)
		count = 0;
	for (int& level : levels)
		level = 1;
	for (bool& f : found)
		f = false;
	for (int& stamp : stamps)
		stamp = 0;

	std::string tok;
	in >> tok;
	int type = ItemType::MELEE_WEAPON;
	int id = 0;
	const bool hasStamps = tok == "INV6";
	bool hasFound = tok == "INV3" || tok == "INV4" || tok == "INV5" || hasStamps;
	const bool resistance = tok == "INV5" || hasStamps;
	int wornType = ItemType::EMPTY;
	int wornId = 0;
	if (tok == "INV2" || tok == "INV3" || hasFound) {
		int slots = 0;
		in >> slots;
		std::vector<int> saved(static_cast<size_t>(slots > 0 && slots <= 64 ? slots : 0));
		for (int& count : saved)
			in >> count;
		for (size_t slot = 0; slot < saved.size(); slot++)
			if (std::optional<ItemKind> kind = savedSlot(slot, saved.size(), resistance))
				counts[itemIndex(*kind)] = saved[slot];
		for (int& level : saved)
			in >> level;
		for (size_t slot = 0; slot < saved.size(); slot++)
			if (std::optional<ItemKind> kind =
					savedSlot(slot, saved.size(), resistance)) // saves from before the cap of 5
				levels[itemIndex(*kind)] = std::clamp(saved[slot], 1, MAX_WEAPON_LEVEL);
		if (hasFound) {
			for (int& f : saved)
				in >> f;
			for (size_t slot = 0; slot < saved.size(); slot++)
				if (std::optional<ItemKind> kind = savedSlot(slot, saved.size(), resistance))
					found[itemIndex(*kind)] = saved[slot] != 0;
		}
		if (hasStamps) {
			for (int& stamp : saved)
				in >> stamp;
			for (size_t slot = 0; slot < saved.size(); slot++)
				if (std::optional<ItemKind> kind = savedSlot(slot, saved.size(), resistance))
					stamps[itemIndex(*kind)] = std::max(0, saved[slot]);
		}
		in >> type >> id;
		if (tok == "INV4" || resistance)
			in >> wornType >> wornId;
	} else {
		counts[itemIndex(OLD_SLOTS[0])] = std::stoi(tok);
		for (int slot = 1; slot < LEGACY_SLOT_COUNT; slot++)
			in >> counts[itemIndex(OLD_SLOTS[slot])];

		// equipped.type/id were added after older saves were written.
		// Old saves have mapX (a float like "3.32501") at this position.
		// Peek at the next token: if it contains '.', it's mapX — rewind and skip.
		auto pos = in.tellg();
		if ((in >> tok) && tok.find('.') == std::string::npos) {
			type = std::stoi(tok);
			if (!(in >> id))
				in.clear();
		} else {
			in.clear();
			in.seekg(pos);
		}
	}

	for (int slot = 0; slot < ITEM_KIND_COUNT; slot++)
		found[slot] = found[slot] || counts[slot] > 0;
	lastStamp = *std::max_element(std::begin(stamps), std::end(stamps));

	std::optional<ItemKind> kind = itemFromFile(type, id);
	equipped = kind && isWeapon(*kind) ? *kind : ItemKind::Club;
	std::optional<ItemKind> on = itemFromFile(wornType, wornId);
	worn = on && isAmulet(*on) && counts[itemIndex(*on)] > 0 ? on : std::nullopt;
}

void ItemBag::Find(ItemKind kind, Journal& journal) {
	Add(kind);
	if (isAmulet(kind)) // no journal note yet
		return;
	if (isPotion(kind)) {
		journal.LearnNote(potionNote(kind));
		return;
	}
	journal.LearnNote(FieldNote::Weapons);
	journal.LearnNote(damageNote(mainType(weaponMix(kind))));
}
