#include "loot.h"
#include "../ui/inventory.h"
#include <array>
#include <cstdlib>

namespace {
// Independent rolls, in percent.
constexpr int SMALL_STAMINA_CHANCE = 30;
constexpr int LARGE_STAMINA_CHANCE = 20;
constexpr int LOWER_WEAPON_CHANCE = 5;	// weapon chests: one weapon of a lower grade
constexpr int SAME_POTION_CHANCE = 5;	// potion chests: a second one of the same potion
constexpr int SMALL_HEALTH_CHANCE = 10; // large health chests only

// Weapons from the weakest up (by base damage).
constexpr std::array<LootItem, 4> WEAPON_GRADES = {{
	{ItemType::MELEE_WEAPON, WeaponId::CLUB},
	{ItemType::RANGED_WEAPON, WeaponId::BOW},
	{ItemType::MELEE_WEAPON, WeaponId::SPEAR},
	{ItemType::MELEE_WEAPON, WeaponId::SWORD},
}};

bool chance(int percent) { return rand() % 100 < percent; }

int weaponGrade(int type, int id) {
	for (size_t grade = 0; grade < WEAPON_GRADES.size(); grade++)
		if (WEAPON_GRADES[grade].type == type && WEAPON_GRADES[grade].id == id)
			return static_cast<int>(grade);
	return -1;
}
} // namespace

std::vector<LootItem> RollChestLoot(int type, int id) {
	std::vector<LootItem> loot = {{type, id}};

	if (chance(SMALL_STAMINA_CHANCE))
		loot.push_back({ItemType::POTION, PotionId::SMALL_STAMINA});
	if (chance(LARGE_STAMINA_CHANCE))
		loot.push_back({ItemType::POTION, PotionId::LARGE_STAMINA});

	if (type == ItemType::POTION) {
		if (chance(SAME_POTION_CHANCE))
			loot.push_back({type, id});
		if (id == PotionId::LARGE_HEALTH && chance(SMALL_HEALTH_CHANCE))
			loot.push_back({ItemType::POTION, PotionId::SMALL_HEALTH});
		return loot;
	}

	int grade = weaponGrade(type, id);
	if (grade > 0 && chance(LOWER_WEAPON_CHANCE))
		loot.push_back(WEAPON_GRADES[static_cast<size_t>(rand() % grade)]);
	return loot;
}
