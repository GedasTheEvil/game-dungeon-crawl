#include "item_bag.h"
#include "journal.h"
#include <algorithm>
#include <cmath>
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

// The item a saved slot holds: a save with ORDERED_SAVE_SLOTS or more slots is in ItemKind order (newer kinds were
// added at the end), an older one in OLD_SLOTS order.
std::optional<ItemKind> savedSlot(size_t slot, size_t slots) {
	if (slots >= ORDERED_SAVE_SLOTS)
		return slot < ITEM_KIND_COUNT ? std::optional(itemAt(static_cast<int>(slot))) : std::nullopt;
	if (slot < OLD_SLOT_COUNT)
		return OLD_SLOTS[slot];
	return std::nullopt;
}

bool heals(ItemKind kind) { return kind == ItemKind::SmallHealth || kind == ItemKind::LargeHealth; }
bool restoresStamina(ItemKind kind) { return kind == ItemKind::SmallStamina || kind == ItemKind::LargeStamina; }
} // namespace

int upgradeCost(int level) { return 1 + level * (level + 1) / 2; }

int weaponGrowthPercent(ItemKind weapon) {
	switch (weapon) {
	case ItemKind::Club:
		return 40;
	case ItemKind::ShortSword:
		return 10;
	case ItemKind::Dagger:
		return 30;
	default:
		return 20;
	}
}

int weaponDamage(ItemKind weapon, int baseDamage, int level) {
	const int percent = 100 + weaponGrowthPercent(weapon) * (level - 1);
	return static_cast<int>(std::lround(static_cast<double>(baseDamage) * percent / 100.0));
}

PotionGain potionGain(ItemKind potion) {
	PotionGain gain;
	switch (potion) {
	case ItemKind::SmallHealth:
		gain.healPercent = PotionEffect::SMALL_HEAL_PERCENT;
		break;
	case ItemKind::LargeHealth:
		gain.healPercent = PotionEffect::LARGE_HEAL_PERCENT;
		break;
	case ItemKind::Might:
		gain.might = PotionEffect::MIGHT;
		break;
	case ItemKind::Armor:
		gain.armor = PotionEffect::ARMOR;
		break;
	case ItemKind::Life:
		gain.maxHpPercent = PotionEffect::LIFE_MAX_HP_PERCENT;
		break;
	case ItemKind::SmallStamina:
		gain.staminaPercent = PotionEffect::SMALL_STAMINA_PERCENT;
		break;
	case ItemKind::LargeStamina:
		gain.staminaPercent = PotionEffect::LARGE_STAMINA_PERCENT;
		break;
	case ItemKind::Antidote:
		gain.cure = true;
		break;
	default: // the weapons
		break;
	}
	return gain;
}

void ItemBag::Reset() {
	for (int& count : counts)
		count = 0;
	for (bool& f : found)
		f = false;
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

bool ItemBag::CanUpgrade(ItemKind kind, bool alive) const {
	return isWeapon(kind) && alive && Level(kind) < MAX_WEAPON_LEVEL && Count(kind) >= upgradeCost(Level(kind));
}

bool ItemBag::Upgrade(ItemKind kind, bool alive) {
	if (!CanUpgrade(kind, alive))
		return false;
	levels[itemIndex(kind)]++;
	return true;
}

std::optional<ItemKind> ItemBag::QuickChoice(QuickKind kind, int current, int max) const {
	bool health = kind == QuickKind::Health;
	int small = Count(health ? ItemKind::SmallHealth : ItemKind::SmallStamina);
	int large = Count(health ? ItemKind::LargeHealth : ItemKind::LargeStamina);
	return quickPotion(kind, current, max, small, large);
}

void ItemBag::Save(std::ostream& out) const {
	out << "INV4 " << ITEM_KIND_COUNT << " ";
	for (int count : counts)
		out << count << " ";
	for (int level : levels)
		out << level << " ";
	for (bool f : found)
		out << (f ? 1 : 0) << " ";
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

	std::string tok;
	in >> tok;
	int type = ItemType::MELEE_WEAPON;
	int id = 0;
	bool hasFound = tok == "INV3" || tok == "INV4";
	int wornType = ItemType::EMPTY;
	int wornId = 0;
	if (tok == "INV2" || tok == "INV3" || tok == "INV4") {
		int slots = 0;
		in >> slots;
		std::vector<int> saved(static_cast<size_t>(slots > 0 && slots <= 64 ? slots : 0));
		for (int& count : saved)
			in >> count;
		for (size_t slot = 0; slot < saved.size(); slot++)
			if (std::optional<ItemKind> kind = savedSlot(slot, saved.size()))
				counts[itemIndex(*kind)] = saved[slot];
		for (int& level : saved)
			in >> level;
		for (size_t slot = 0; slot < saved.size(); slot++)
			if (std::optional<ItemKind> kind = savedSlot(slot, saved.size())) // saves from before the cap of 5
				levels[itemIndex(*kind)] = std::clamp(saved[slot], 1, MAX_WEAPON_LEVEL);
		if (hasFound) {
			for (int& f : saved)
				in >> f;
			for (size_t slot = 0; slot < saved.size(); slot++)
				if (std::optional<ItemKind> kind = savedSlot(slot, saved.size()))
					found[itemIndex(*kind)] = saved[slot] != 0;
		}
		in >> type >> id;
		if (tok == "INV4")
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
