#include "../../external/doctest/doctest.h"
#include "../../src/world/loot.h"

TEST_CASE("the same seed rolls the same loot") {
	Rng a(7);
	Rng b(7);
	for (int i = 0; i < 50; i++)
		CHECK(RollChestLoot(ItemKind::Sword, a) == RollChestLoot(ItemKind::Sword, b));
}

TEST_CASE("a chest holds its item first, then bonuses") {
	Rng rng(3);
	int bonuses = 0;
	for (int i = 0; i < 1000; i++) {
		std::vector<ItemKind> loot = RollChestLoot(ItemKind::LargeHealth, rng);
		REQUIRE_FALSE(loot.empty());
		CHECK(loot.front() == ItemKind::LargeHealth);
		for (size_t k = 1; k < loot.size(); k++)
			CHECK(isPotion(loot[k])); // a potion chest's bonuses are potions
		bonuses += static_cast<int>(loot.size()) - 1;
	}
	// Small stamina 30%, large stamina 20%, the same potion 5%, a small health 10%: about 0.65 per chest.
	CHECK(bonuses > 500);
	CHECK(bonuses < 800);
}

TEST_CASE("a weapon chest may add a weaker weapon, never a better one") {
	Rng rng(11);
	for (int i = 0; i < 2000; i++)
		for (ItemKind item : RollChestLoot(ItemKind::Spear, rng))
			CHECK(item != ItemKind::Sword);
}

TEST_CASE("the mimic's chest holds any item but the antidote") {
	Rng rng(5);
	bool seen[ITEM_KIND_COUNT] = {};
	for (int i = 0; i < 2000; i++)
		seen[itemIndex(RollMimicLoot(rng))] = true;
	for (int i = 0; i < ITEM_KIND_COUNT; i++)
		CHECK(seen[i] == (itemAt(i) != ItemKind::Antidote));
}
