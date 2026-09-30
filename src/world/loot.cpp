#include "loot.h"
#include <array>

namespace {
// Independent rolls, in percent.
constexpr int SMALL_STAMINA_CHANCE = 30;
constexpr int LARGE_STAMINA_CHANCE = 20;
constexpr int LOWER_WEAPON_CHANCE = 5;	// weapon chests: one weapon of a lower grade
constexpr int SAME_POTION_CHANCE = 5;	// potion chests: a second one of the same potion
constexpr int SMALL_HEALTH_CHANCE = 10; // large health chests only

// Weapons from the weakest up (by base damage).
constexpr std::array<ItemKind, WEAPON_KIND_COUNT> WEAPON_GRADES = {
	{ItemKind::Club, ItemKind::Bow, ItemKind::Spear, ItemKind::Sword}};

int weaponGrade(ItemKind kind) {
	for (size_t grade = 0; grade < WEAPON_GRADES.size(); grade++)
		if (WEAPON_GRADES[grade] == kind)
			return static_cast<int>(grade);
	return -1;
}
} // namespace

std::vector<ItemKind> RollChestLoot(ItemKind placed, Rng& rng) {
	std::vector<ItemKind> loot = {placed};

	if (rng.percent(SMALL_STAMINA_CHANCE))
		loot.push_back(ItemKind::SmallStamina);
	if (rng.percent(LARGE_STAMINA_CHANCE))
		loot.push_back(ItemKind::LargeStamina);

	if (isPotion(placed)) {
		if (rng.percent(SAME_POTION_CHANCE))
			loot.push_back(placed);
		if (placed == ItemKind::LargeHealth && rng.percent(SMALL_HEALTH_CHANCE))
			loot.push_back(ItemKind::SmallHealth);
		return loot;
	}

	int grade = weaponGrade(placed);
	if (grade > 0 && rng.percent(LOWER_WEAPON_CHANCE))
		loot.push_back(WEAPON_GRADES[static_cast<size_t>(rng.below(grade))]);
	return loot;
}

ItemKind RollMimicLoot(Rng& rng) {
	if (rng.percent(50))
		return WEAPON_GRADES[static_cast<size_t>(rng.below(WEAPON_KIND_COUNT))];
	return itemAt(WEAPON_KIND_COUNT + rng.below(POTION_KIND_COUNT));
}
