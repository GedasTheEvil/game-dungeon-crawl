#include "../../external/doctest/doctest.h"
#include "../../src/world/item_bag.h"
#include "../../src/world/items.h"
#include "../../src/world/quick_potion.h"
#include <sstream>

TEST_CASE("every item converts to its file id and back") {
	for (int i = 0; i < ITEM_KIND_COUNT; i++) {
		ItemFileId file = fileIdOf(itemAt(i));
		REQUIRE(itemFromFile(file.type, file.id).has_value());
		CHECK(*itemFromFile(file.type, file.id) == itemAt(i));
	}
}

TEST_CASE("file ids as the level files use them") {
	CHECK(*itemFromFile(ItemType::MELEE_WEAPON, 0) == ItemKind::Club);
	CHECK(*itemFromFile(ItemType::MELEE_WEAPON, 2) == ItemKind::Spear);
	CHECK(*itemFromFile(ItemType::RANGED_WEAPON, 0) == ItemKind::Bow);
	CHECK(*itemFromFile(ItemType::POTION, 0) == ItemKind::SmallHealth);
	CHECK(*itemFromFile(ItemType::POTION, 6) == ItemKind::LargeStamina);
	CHECK_FALSE(itemFromFile(ItemType::EMPTY, 0).has_value());
	CHECK_FALSE(itemFromFile(ItemType::POTION, 7).has_value());
	CHECK_FALSE(itemFromFile(ItemType::RANGED_WEAPON, 1).has_value());
}

TEST_CASE("weapons before potions") {
	CHECK_FALSE(isPotion(ItemKind::Bow));
	CHECK(isPotion(ItemKind::SmallHealth));
	CHECK(isRanged(ItemKind::Bow));
	CHECK_FALSE(isRanged(ItemKind::Spear));
}

TEST_CASE("weapon levels") {
	CHECK(upgradeCost(1) == 2);
	CHECK(upgradeCost(3) == 7);
	CHECK(upgradeCost(4) == 11);
	CHECK(weaponDamage(ItemKind::Sword, 35, 1) == 35);
	CHECK(weaponDamage(ItemKind::Sword, 35, 2) == 39); // +10%
	CHECK(weaponDamage(ItemKind::Club, 10, 5) == 26);  // +40% a level: 10 x 2.6
	CHECK(weaponDamage(ItemKind::Spear, 20, 3) == 28); // +20% a level
}

TEST_CASE("a new bag holds the club, in hand") {
	ItemBag bag;
	CHECK(bag.Count(ItemKind::Club) == 1);
	CHECK(bag.Equipped() == ItemKind::Club);
	CHECK(bag.Level(ItemKind::Sword) == 1);
	for (int i = 1; i < ITEM_KIND_COUNT; i++)
		CHECK(bag.Count(itemAt(i)) == 0);
}

TEST_CASE("what can be used") {
	ItemBag bag;
	Vitals hurt{true, 50, 100, 100, 100};
	CHECK(bag.Block(ItemKind::Club, hurt) == UseBlock::Equipped);
	CHECK(bag.Block(ItemKind::Sword, hurt) == UseBlock::NotFound);
	CHECK(bag.Block(ItemKind::SmallHealth, hurt) == UseBlock::NoneLeft);

	bag.Add(ItemKind::SmallHealth);
	bag.Add(ItemKind::SmallStamina);
	CHECK(bag.Block(ItemKind::SmallHealth, hurt) == UseBlock::None);
	CHECK(bag.Block(ItemKind::SmallStamina, hurt) == UseBlock::StaminaFull);
	CHECK(bag.Block(ItemKind::SmallHealth, Vitals{true, 100, 100, 10, 100}) == UseBlock::HealthFull);
	CHECK(bag.Block(ItemKind::SmallHealth, Vitals{false, 50, 100, 10, 100}) == UseBlock::Dead);
}

TEST_CASE("using takes a potion out and equips a weapon") {
	ItemBag bag;
	Vitals hurt{true, 50, 100, 100, 100};
	bag.Add(ItemKind::SmallHealth);
	bag.Add(ItemKind::Spear);
	CHECK(bag.Use(ItemKind::SmallHealth, hurt));
	CHECK(bag.Count(ItemKind::SmallHealth) == 0);
	CHECK_FALSE(bag.Use(ItemKind::SmallHealth, hurt));
	CHECK(bag.Use(ItemKind::Spear, hurt));
	CHECK(bag.Equipped() == ItemKind::Spear);
	CHECK(bag.Count(ItemKind::Spear) == 1); // equipping keeps it
}

TEST_CASE("upgrades take enough copies, up to the max level") {
	ItemBag bag;
	bag.Add(ItemKind::Sword);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::Sword, true)); // 1 of 2
	bag.Add(ItemKind::Sword);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::Sword, false)); // dead
	CHECK(bag.Upgrade(ItemKind::Sword, true));
	CHECK(bag.Level(ItemKind::Sword) == 2);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::Sword, true)); // 2 of 4
	for (int i = 0; i < 20; i++)
		bag.Add(ItemKind::Sword);
	while (bag.Upgrade(ItemKind::Sword, true)) {
	}
	CHECK(bag.Level(ItemKind::Sword) == MAX_WEAPON_LEVEL);
	bag.Add(ItemKind::SmallHealth);
	bag.Add(ItemKind::SmallHealth);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::SmallHealth, true)); // potions have no levels
}

TEST_CASE("potion gains") {
	CHECK(potionGain(ItemKind::SmallHealth).healPercent == 25);
	CHECK(potionGain(ItemKind::LargeHealth).healPercent == 50);
	CHECK(potionGain(ItemKind::Might).might == 2);
	CHECK(potionGain(ItemKind::Armor).armor == 2);
	CHECK(potionGain(ItemKind::Life).maxHpPercent == 5);
	CHECK(potionGain(ItemKind::SmallStamina).staminaPercent == 50);
	CHECK(potionGain(ItemKind::LargeStamina).staminaPercent == 100);
	CHECK(potionGain(ItemKind::Sword).healPercent == 0);
}

TEST_CASE("the quick heal takes the weakest potion that gets the player out of danger") {
	// 100 max: danger under 35. Small heals 25.
	CHECK(*quickPotion(QuickKind::Health, 50, 100, 1, 1) == ItemKind::SmallHealth);
	CHECK(*quickPotion(QuickKind::Health, 5, 100, 1, 1) == ItemKind::LargeHealth); // 5 + 25 = 30 < 35
	CHECK(*quickPotion(QuickKind::Health, 5, 100, 1, 0) == ItemKind::SmallHealth);
	CHECK(*quickPotion(QuickKind::Health, 50, 100, 0, 1) == ItemKind::LargeHealth);
	CHECK_FALSE(quickPotion(QuickKind::Health, 50, 100, 0, 0).has_value());
	CHECK_FALSE(quickPotion(QuickKind::Health, 100, 100, 1, 1).has_value());
	CHECK(*quickPotion(QuickKind::Stamina, 10, 100, 1, 1) == ItemKind::SmallStamina);
}

TEST_CASE("the bag survives a save and a load") {
	ItemBag bag;
	bag.Add(ItemKind::Bow);
	bag.Add(ItemKind::Bow);
	bag.Add(ItemKind::LargeStamina);
	CHECK(bag.Upgrade(ItemKind::Bow, true));
	CHECK(bag.Use(ItemKind::Bow, Vitals{true, 1, 1, 1, 1}));
	std::stringstream file;
	bag.Save(file);
	CHECK(file.str().rfind("INV2 11 ", 0) == 0);

	ItemBag loaded;
	loaded.Load(file);
	for (int i = 0; i < ITEM_KIND_COUNT; i++) {
		CHECK(loaded.Count(itemAt(i)) == bag.Count(itemAt(i)));
		CHECK(loaded.Level(itemAt(i)) == bag.Level(itemAt(i)));
	}
	CHECK(loaded.Equipped() == ItemKind::Bow);
}

TEST_CASE("an old save: 9 counts, then the equipped weapon") {
	std::stringstream file("1 1 0 0 3 0 0 0 0 1 1\n");
	ItemBag bag;
	bag.Load(file);
	CHECK(bag.Count(ItemKind::Club) == 1);
	CHECK(bag.Count(ItemKind::Sword) == 1);
	CHECK(bag.Count(ItemKind::SmallHealth) == 3);
	CHECK(bag.Count(ItemKind::SmallStamina) == 0);
	CHECK(bag.Equipped() == ItemKind::Sword);
}

TEST_CASE("an older save: 9 counts, then the map position") {
	std::stringstream file("1 0 0 0 0 0 0 0 0 3.32501 4\n");
	ItemBag bag;
	bag.Load(file);
	CHECK(bag.Equipped() == ItemKind::Club);
	float mapX = 0.f;
	file >> mapX; // left for the dungeon to read
	CHECK(mapX == doctest::Approx(3.32501f));
}
