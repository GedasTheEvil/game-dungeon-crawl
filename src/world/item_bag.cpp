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

bool heals(ItemKind kind) { return kind == ItemKind::SmallHealth || kind == ItemKind::LargeHealth; }
bool restoresStamina(ItemKind kind) { return kind == ItemKind::SmallStamina || kind == ItemKind::LargeStamina; }
} // namespace

int upgradeCost(int level) { return 1 + level * (level + 1) / 2; }

int weaponGrowthPercent(ItemKind weapon) {
	switch (weapon) {
	case ItemKind::Club:
		return 40;
	case ItemKind::Sword:
		return 10;
	default: // spear, bow
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
	case ItemKind::Club:
	case ItemKind::Sword:
	case ItemKind::Spear:
	case ItemKind::Bow:
		break;
	}
	return gain;
}

void ItemBag::Reset() {
	for (int& count : counts)
		count = 0;
	counts[itemIndex(ItemKind::Club)] = 1; // everyone starts with the club
	for (int& level : levels)
		level = 1;
	equipped = ItemKind::Club;
}

UseBlock ItemBag::Block(ItemKind kind, const Vitals& player) const {
	if (!player.alive)
		return UseBlock::Dead;
	if (Count(kind) <= 0)
		return isPotion(kind) ? UseBlock::NoneLeft : UseBlock::NotFound;
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
	else
		equipped = kind;
	return true;
}

bool ItemBag::CanUpgrade(ItemKind kind, bool alive) const {
	return !isPotion(kind) && alive && Level(kind) < MAX_WEAPON_LEVEL && Count(kind) >= upgradeCost(Level(kind));
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
	out << "INV2 " << ITEM_KIND_COUNT << " ";
	for (int count : counts)
		out << count << " ";
	for (int level : levels)
		out << level << " ";
	ItemFileId id = fileIdOf(equipped);
	out << id.type << " " << id.id << "\n";
}

void ItemBag::Load(std::istream& in) {
	for (int& count : counts)
		count = 0;
	for (int& level : levels)
		level = 1;

	std::string tok;
	in >> tok;
	int type = ItemType::MELEE_WEAPON;
	int id = 0;
	if (tok == "INV2") {
		int slots = 0;
		in >> slots;
		std::vector<int> saved(static_cast<size_t>(slots > 0 && slots <= 64 ? slots : 0));
		for (int& count : saved)
			in >> count;
		for (size_t slot = 0; slot < saved.size() && slot < ITEM_KIND_COUNT; slot++)
			counts[slot] = saved[slot];
		for (int& level : saved)
			in >> level;
		for (size_t slot = 0; slot < saved.size() && slot < ITEM_KIND_COUNT; slot++)
			levels[slot] = std::clamp(saved[slot], 1, MAX_WEAPON_LEVEL); // saves from before the cap of 5
		in >> type >> id;
	} else {
		counts[0] = std::stoi(tok);
		for (int slot = 1; slot < LEGACY_SLOT_COUNT; slot++)
			in >> counts[slot];

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

	std::optional<ItemKind> kind = itemFromFile(type, id);
	equipped = kind && !isPotion(*kind) ? *kind : ItemKind::Club;
}

void ItemBag::Find(ItemKind kind, Journal& journal) {
	Add(kind);
	if (!isPotion(kind))
		journal.LearnNote(FieldNote::Weapons);
}
