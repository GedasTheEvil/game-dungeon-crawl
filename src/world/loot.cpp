#include "loot.h"
#include <algorithm>
#include <array>

namespace {
// Independent rolls, in percent.
constexpr int SMALL_STAMINA_CHANCE = 30;
constexpr int LARGE_STAMINA_CHANCE = 20;
constexpr int LOWER_WEAPON_CHANCE = 5;	// weapon chests: one weapon of a lower grade
constexpr int SAME_POTION_CHANCE = 5;	// potion chests: a second one of the same potion
constexpr int SMALL_HEALTH_CHANCE = 10; // large health chests only

// Weapons from the weakest up, by base damage (WeaponDef::damage); equal ones in ItemKind order.
const std::array<ItemKind, WEAPON_KIND_COUNT> WEAPON_GRADES = [] {
	std::array<ItemKind, WEAPON_KIND_COUNT> grades{};
	for (int i = 0; i < WEAPON_KIND_COUNT; i++)
		grades[static_cast<size_t>(i)] = itemAt(i);
	std::stable_sort(grades.begin(), grades.end(),
					 [](ItemKind a, ItemKind b) { return weaponDef(a).damage < weaponDef(b).damage; });
	return grades;
}();

int weaponGrade(ItemKind kind) {
	for (size_t grade = 0; grade < WEAPON_GRADES.size(); grade++)
		if (WEAPON_GRADES[grade] == kind)
			return static_cast<int>(grade);
	return -1;
}

// One of the weapons `owned` whose grade is below `grades`, at random; nullopt if none.
std::optional<ItemKind> pickOwned(const OwnedWeapons& owned, int grades, Rng& rng) {
	int count = 0;
	for (int grade = 0; grade < grades; grade++)
		count += owned[static_cast<size_t>(itemIndex(WEAPON_GRADES[static_cast<size_t>(grade)]))] ? 1 : 0;
	if (count == 0)
		return std::nullopt;
	int pick = rng.below(count);
	for (int grade = 0; grade < grades; grade++) {
		const ItemKind kind = WEAPON_GRADES[static_cast<size_t>(grade)];
		if (owned[static_cast<size_t>(itemIndex(kind))] && pick-- == 0)
			return kind;
	}
	return std::nullopt;
}
} // namespace

std::optional<ItemKind> RollChestAmulet(int level, Rng& rng) {
	if (!rng.percent(CHEST_AMULET_CHANCE))
		return std::nullopt;
	const bool minor = level >= MINOR_AMULET_LEVEL && rng.percent(MINOR_AMULET_CHANCE);
	// Every type but regeneration has a lesser and a minor tier.
	int pick = rng.below(AMULET_TYPE_COUNT - 1);
	if (pick >= static_cast<int>(AmuletType::Regeneration))
		pick++;
	const auto type = static_cast<AmuletType>(pick);
	return amuletKind(type, minor ? AmuletTier::Minor : AmuletTier::Lesser);
}

ItemKind RollBossAmulet(int level, Rng& rng) {
	const bool grand = rng.percent(BOSS_GRAND_PERCENT_PER_LEVEL * level);
	const auto type = static_cast<AmuletType>(rng.below(AMULET_TYPE_COUNT));
	return amuletKind(type, grand ? AmuletTier::Grand : AmuletTier::Normal).value_or(ItemKind::StrengthNormal);
}

std::vector<ItemKind> RollChestLoot(ItemKind placed, const OwnedWeapons& owned, int level, Rng& rng) {
	std::vector<ItemKind> loot = {placed};
	if (isAmulet(placed))
		return loot;
	// Rolled last, so the other bonuses come out as they did before the amulets.
	auto withAmulet = [&]() -> std::vector<ItemKind>& {
		if (std::optional<ItemKind> amulet = RollChestAmulet(level, rng))
			loot.push_back(*amulet);
		return loot;
	};

	if (rng.percent(SMALL_STAMINA_CHANCE))
		loot.push_back(ItemKind::SmallStamina);
	if (rng.percent(LARGE_STAMINA_CHANCE))
		loot.push_back(ItemKind::LargeStamina);

	if (isPotion(placed)) {
		if (rng.percent(SAME_POTION_CHANCE))
			loot.push_back(placed);
		if (placed == ItemKind::LargeHealth && rng.percent(SMALL_HEALTH_CHANCE))
			loot.push_back(ItemKind::SmallHealth);
		return withAmulet();
	}

	int grade = weaponGrade(placed);
	if (grade > 0 && rng.percent(LOWER_WEAPON_CHANCE))
		if (std::optional<ItemKind> lower = pickOwned(owned, grade, rng))
			loot.push_back(*lower);
	return withAmulet();
}

ItemKind RollMimicLoot(const OwnedWeapons& owned, Rng& rng) {
	if (rng.percent(50))
		return pickOwned(owned, WEAPON_KIND_COUNT, rng).value_or(ItemKind::Club);
	int count = 0;
	for (int i = WEAPON_KIND_COUNT; i < FIRST_AMULET; i++)
		count += potionDef(itemAt(i)).mimicLoot ? 1 : 0;
	int pick = rng.below(count);
	for (int i = WEAPON_KIND_COUNT; i < FIRST_AMULET; i++)
		if (potionDef(itemAt(i)).mimicLoot && pick-- == 0)
			return itemAt(i);
	return ItemKind::SmallHealth;
}

std::optional<ItemKind> RollKillDrop(bool boss, const OwnedWeapons& owned, int level, Rng& rng) {
	if (boss)
		return RollBossAmulet(level, rng);
	if (!rng.percent(KILL_DROP_CHANCE))
		return std::nullopt;
	return pickOwned(owned, WEAPON_KIND_COUNT, rng);
}
