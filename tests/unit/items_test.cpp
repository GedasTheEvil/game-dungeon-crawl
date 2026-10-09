#include "../../external/doctest/doctest.h"
#include "../../src/world/item_bag.h"
#include "../../src/world/items.h"
#include "../../src/world/quick_potion.h"
#include <sstream>
#include <string>
#include <vector>

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
	CHECK(*itemFromFile(ItemType::RANGED_WEAPON, 0) == ItemKind::SelfBow);
	CHECK(*itemFromFile(ItemType::POTION, 0) == ItemKind::SmallHealth);
	CHECK(*itemFromFile(ItemType::POTION, 6) == ItemKind::LargeStamina);
	CHECK_FALSE(itemFromFile(ItemType::EMPTY, 0).has_value());
	CHECK(*itemFromFile(ItemType::POTION, 7) == ItemKind::Antidote);
	CHECK(*itemFromFile(ItemType::POTION, 8) == ItemKind::LesserResistance);
	CHECK(*itemFromFile(ItemType::POTION, 9) == ItemKind::GreaterResistance);
	CHECK_FALSE(itemFromFile(ItemType::POTION, 10).has_value());
	CHECK(*itemFromFile(ItemType::RANGED_WEAPON, 1) == ItemKind::CompositeBow);
	CHECK(*itemFromFile(ItemType::RANGED_WEAPON, 4) == ItemKind::Javelin);
	CHECK_FALSE(itemFromFile(ItemType::RANGED_WEAPON, 5).has_value());
}

TEST_CASE("weapons before potions") {
	CHECK_FALSE(isPotion(ItemKind::SelfBow));
	CHECK(isPotion(ItemKind::SmallHealth));
	CHECK(isRanged(ItemKind::SelfBow));
	CHECK_FALSE(isRanged(ItemKind::Spear));
}

TEST_CASE("weapon levels") {
	CHECK(upgradeCost(1) == 2);
	CHECK(upgradeCost(3) == 7);
	CHECK(upgradeCost(4) == 11);
	CHECK(weaponDamage(ItemKind::ShortSword, 35, 1) == 35);
	CHECK(weaponDamage(ItemKind::ShortSword, 35, 2) == 39); // +10%
	CHECK(weaponDamage(ItemKind::Club, 10, 5) == 26);		// +40% a level: 10 x 2.6
	CHECK(weaponDamage(ItemKind::Spear, 20, 3) == 28);		// +20% a level
}

TEST_CASE("a new bag holds the club, in hand") {
	ItemBag bag;
	CHECK(bag.Count(ItemKind::Club) == 1);
	CHECK(bag.Equipped() == ItemKind::Club);
	CHECK(bag.Level(ItemKind::ShortSword) == 1);
	for (int i = 1; i < ITEM_KIND_COUNT; i++)
		CHECK(bag.Count(itemAt(i)) == 0);
}

TEST_CASE("what can be used") {
	ItemBag bag;
	Vitals hurt{true, 50, 100, 100, 100};
	CHECK(bag.Block(ItemKind::Club, hurt) == UseBlock::Equipped);
	CHECK(bag.Block(ItemKind::ShortSword, hurt) == UseBlock::NotFound);
	CHECK(bag.Block(ItemKind::SmallHealth, hurt) == UseBlock::NotFound); // never had one

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
	bag.Add(ItemKind::ShortSword);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::ShortSword, true)); // 1 of 2
	bag.Add(ItemKind::ShortSword);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::ShortSword, false)); // dead
	CHECK(bag.Upgrade(ItemKind::ShortSword, true));
	CHECK(bag.Level(ItemKind::ShortSword) == 2);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::ShortSword, true)); // 2 of 4
	for (int i = 0; i < 20; i++)
		bag.Add(ItemKind::ShortSword);
	while (bag.Upgrade(ItemKind::ShortSword, true)) {
	}
	CHECK(bag.Level(ItemKind::ShortSword) == MAX_WEAPON_LEVEL);
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
	CHECK(potionGain(ItemKind::Antidote).cure);
	CHECK(potionGain(ItemKind::Antidote).healPercent == 0);
	CHECK(potionGain(ItemKind::LesserResistance).resistPercent == 50);
	CHECK_FALSE(potionGain(ItemKind::LesserResistance).cure);
	CHECK(potionGain(ItemKind::GreaterResistance).resistPercent == 95);
	CHECK(potionGain(ItemKind::GreaterResistance).cure);
	CHECK(potionGain(ItemKind::ShortSword).healPercent == 0);
}

TEST_CASE("the antidote is drunk only while poisoned") {
	ItemBag bag;
	bag.Add(ItemKind::Antidote);
	Vitals healthy{true, 50, 50, 100, 100};
	CHECK(bag.Block(ItemKind::Antidote, healthy) == UseBlock::NotPoisoned);
	Vitals poisoned = healthy;
	poisoned.poisoned = true;
	CHECK(bag.Use(ItemKind::Antidote, poisoned));
	CHECK(bag.Count(ItemKind::Antidote) == 0);
}

TEST_CASE("a lesser resistance potion does not cut a greater one short") {
	ItemBag bag;
	bag.Add(ItemKind::LesserResistance);
	bag.Add(ItemKind::GreaterResistance);
	Vitals greater{true, 50, 50, 100, 100};
	greater.resistPercent = 95;
	CHECK(bag.Block(ItemKind::LesserResistance, greater) == UseBlock::StrongerResistance);
	CHECK(bag.Block(ItemKind::GreaterResistance, greater) == UseBlock::None);
	Vitals lesser = greater;
	lesser.resistPercent = 50;
	CHECK(bag.Block(ItemKind::LesserResistance, lesser) == UseBlock::None);
	CHECK(bag.Block(ItemKind::GreaterResistance, lesser) == UseBlock::None);
}

TEST_CASE("a potion used up is found, not one never had") {
	ItemBag bag;
	Vitals player{true, 10, 50, 100, 100};
	CHECK(bag.Block(ItemKind::SmallHealth, player) == UseBlock::NotFound);
	CHECK_FALSE(bag.AnyFound(ItemGroup::Potions));
	bag.Add(ItemKind::SmallHealth);
	CHECK(bag.Use(ItemKind::SmallHealth, player));
	CHECK(bag.Block(ItemKind::SmallHealth, player) == UseBlock::NoneLeft);
	CHECK(bag.AnyFound(ItemGroup::Potions));
	CHECK(bag.AnyFound(ItemGroup::Weapons)); // the club
	CHECK_FALSE(bag.AnyFound(ItemGroup::Amulets));

	std::stringstream file;
	bag.Save(file);
	ItemBag loaded;
	loaded.Load(file);
	CHECK(loaded.Found(ItemKind::SmallHealth));
	CHECK(loaded.Count(ItemKind::SmallHealth) == 0);
	CHECK_FALSE(loaded.Found(ItemKind::LargeHealth));
}

TEST_CASE("the item groups follow the ItemKind order") {
	CHECK(groupItems(ItemGroup::Weapons).first == 0);
	CHECK(groupItems(ItemGroup::Weapons).count == WEAPON_KIND_COUNT);
	CHECK(groupItems(ItemGroup::Potions).first == WEAPON_KIND_COUNT);
	CHECK(groupItems(ItemGroup::Potions).count == POTION_KIND_COUNT);
	CHECK(groupItems(ItemGroup::Amulets).first == FIRST_AMULET);
	CHECK(groupItems(ItemGroup::Amulets).count == AMULET_KIND_COUNT);
	CHECK(groupItems(ItemGroup::Rings).count == 0);
}

TEST_CASE("a save from before the antidote loads with none") {
	std::stringstream old("INV2 11 1 0 0 0 2 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 0\n");
	ItemBag bag;
	bag.Load(old);
	CHECK(bag.Count(ItemKind::SmallHealth) == 2);
	CHECK(bag.Count(ItemKind::LargeStamina) == 1);
	CHECK(bag.Count(ItemKind::Antidote) == 0);
	CHECK(bag.Found(ItemKind::SmallHealth));
	CHECK_FALSE(bag.Found(ItemKind::Antidote));
	CHECK(bag.Equipped() == ItemKind::Club);
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
	bag.Add(ItemKind::SelfBow);
	bag.Add(ItemKind::SelfBow);
	bag.Add(ItemKind::LargeStamina);
	CHECK(bag.Upgrade(ItemKind::SelfBow, true));
	CHECK(bag.Use(ItemKind::SelfBow, Vitals{true, 1, 1, 1, 1}));
	std::stringstream file;
	bag.Save(file);
	CHECK(file.str().rfind("INV6 " + std::to_string(ITEM_KIND_COUNT) + " ", 0) == 0);

	ItemBag loaded;
	loaded.Load(file);
	for (int i = 0; i < ITEM_KIND_COUNT; i++) {
		CHECK(loaded.Count(itemAt(i)) == bag.Count(itemAt(i)));
		CHECK(loaded.Level(itemAt(i)) == bag.Level(itemAt(i)));
		CHECK(loaded.Found(itemAt(i)) == bag.Found(itemAt(i)));
		CHECK(loaded.Stamp(itemAt(i)) == bag.Stamp(itemAt(i)));
	}
	CHECK(loaded.Equipped() == ItemKind::SelfBow);
	loaded.Add(ItemKind::Club); // the stamps go on counting from the saved ones
	CHECK(loaded.Stamp(ItemKind::Club) > loaded.Stamp(ItemKind::LargeStamina));
}

TEST_CASE("a save from before the stamps: nothing recent, found first among them") {
	ItemBag bag;
	bag.Add(ItemKind::Antidote);
	bag.Add(ItemKind::SmallHealth);
	std::stringstream file;
	bag.Save(file);
	std::string text = file.str();
	// INV5: the same without the stamps
	std::istringstream words(text.substr(5));
	std::ostringstream old;
	old << "INV5";
	int slots = 0;
	words >> slots;
	old << " " << slots;
	for (int i = 0; i < 4 * slots + 4; i++) {
		std::string word;
		words >> word;
		if (i < 3 * slots || i >= 4 * slots)
			old << " " << word;
	}
	std::stringstream oldFile(old.str());
	ItemBag loaded;
	loaded.Load(oldFile);
	CHECK(loaded.Count(ItemKind::Antidote) == 1);
	CHECK(loaded.Stamp(ItemKind::Antidote) == 0);
	CHECK(tabOrder(loaded, ItemGroup::Potions, SortOrder::Recent).front() == ItemKind::SmallHealth);
	loaded.Add(ItemKind::Antidote);
	CHECK(tabOrder(loaded, ItemGroup::Potions, SortOrder::Recent).front() == ItemKind::Antidote);
}

TEST_CASE("a save from before the resistance potions: the amulets right after the antidote") {
	std::stringstream old;
	old << "INV4 " << ITEM_KIND_COUNT - 2 << " ";
	for (int i = 0; i < ITEM_KIND_COUNT - 2; i++) // counts: 1 antidote, 2 lesser strength amulets, 3 grand venom
		old << (i == itemIndex(ItemKind::Antidote) ? 1
				: i == ORDERED_SAVE_SLOTS		   ? 2
				: i == ITEM_KIND_COUNT - 3		   ? 3
												   : 0)
			<< " ";
	for (int i = 0; i < 2 * (ITEM_KIND_COUNT - 2); i++) // levels, found
		old << (i < ITEM_KIND_COUNT - 2 ? 1 : 0) << " ";
	old << "1 0 " << ItemType::AMULET << " 0\n"; // the club, the lesser strength amulet on
	ItemBag bag;
	bag.Load(old);
	CHECK(bag.Count(ItemKind::Antidote) == 1);
	CHECK(bag.Count(ItemKind::LesserResistance) == 0);
	CHECK(bag.Count(ItemKind::GreaterResistance) == 0);
	CHECK(bag.Count(ItemKind::StrengthLesser) == 2);
	CHECK(bag.Count(ItemKind::VenomGrand) == 3);
	CHECK(bag.Worn() == ItemKind::StrengthLesser);
}

TEST_CASE("a save from before the Egyptian weapons: 12 slots in the old order") {
	// club, sword, spear, bow, the 8 potions; levels; found; the bow in hand
	std::stringstream file("INV3 12 1 0 1 1 2 0 0 0 0 0 0 1  1 1 2 1 1 1 1 1 1 1 1 1  1 0 1 1 1 0 0 0 0 0 0 1  2 0\n");
	ItemBag bag;
	bag.Load(file);
	CHECK(bag.Count(ItemKind::Spear) == 1);
	CHECK(bag.Level(ItemKind::Spear) == 2);
	CHECK(bag.Count(ItemKind::SelfBow) == 1);
	CHECK(bag.Count(ItemKind::SmallHealth) == 2);
	CHECK(bag.Count(ItemKind::Antidote) == 1);
	CHECK(bag.Count(ItemKind::Sling) == 0);
	CHECK(bag.Equipped() == ItemKind::SelfBow);
}

TEST_CASE("an old save: 9 counts, then the equipped weapon") {
	std::stringstream file("1 1 0 0 3 0 0 0 0 1 1\n");
	ItemBag bag;
	bag.Load(file);
	CHECK(bag.Count(ItemKind::Club) == 1);
	CHECK(bag.Count(ItemKind::ShortSword) == 1);
	CHECK(bag.Count(ItemKind::SmallHealth) == 3);
	CHECK(bag.Count(ItemKind::SmallStamina) == 0);
	CHECK(bag.Equipped() == ItemKind::ShortSword);
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

TEST_CASE("amulets: four tiers a type, regeneration only normal and grand") {
	CHECK(AMULET_KIND_COUNT == 9 * AMULET_TIER_COUNT + 2);
	CHECK(*amuletKind(AmuletType::Venom, AmuletTier::Lesser) == ItemKind::VenomLesser);
	CHECK(isAmulet(ItemKind::StrengthLesser));
	CHECK_FALSE(isPotion(ItemKind::StrengthLesser));
	CHECK_FALSE(isWeapon(ItemKind::StrengthLesser));
	CHECK(isPotion(ItemKind::Antidote));
	CHECK(*amuletKind(AmuletType::Armor, AmuletTier::Normal) == ItemKind::ArmorNormal);
	CHECK(*amuletKind(AmuletType::Regeneration, AmuletTier::Grand) == ItemKind::RegenerationGrand);
	CHECK_FALSE(amuletKind(AmuletType::Regeneration, AmuletTier::Minor).has_value());
	for (int i = FIRST_AMULET; i < ITEM_KIND_COUNT; i++) {
		const Amulet a = amuletOf(itemAt(i));
		CHECK(*amuletKind(a.type, a.tier) == itemAt(i));
	}
	CHECK(fileIdOf(ItemKind::StrengthLesser).type == ItemType::AMULET);
	CHECK(fileIdOf(ItemKind::StrengthLesser).id == 0);
	CHECK_FALSE(itemFromFile(ItemType::AMULET, AMULET_KIND_COUNT).has_value());
}

TEST_CASE("amulet names and effects") {
	CHECK(std::string(itemText(ItemKind::StrengthLesser).name) == "Lesser Amulet of Strength");
	CHECK(std::string(itemText(ItemKind::StrengthMinor).name) == "Amulet of Minor Strength");
	CHECK(std::string(itemText(ItemKind::StrengthNormal).name) == "Amulet of Strength");
	CHECK(std::string(itemText(ItemKind::StrengthGrand).name) == "Grand Amulet of Strength");
	CHECK(std::string(itemText(ItemKind::StrengthNormal).effect) == "Might +4 while worn");
	CHECK(std::string(itemText(ItemKind::TrapWardGrand).effect) == "Immune to traps");
	CHECK(std::string(itemText(ItemKind::HealthMinor).label) == "minor amulet of health");
	CHECK(std::string(itemText(ItemKind::VenomGrand).effect) == "30% of hits poison (strong)");
	CHECK(std::string(itemText(ItemKind::VenomLesser).label) == "lesser amulet of venom");
}

TEST_CASE("the bonus of each amulet") {
	CHECK(amuletBonus(std::nullopt).might == 0);
	CHECK(amuletBonus(ItemKind::StrengthLesser).might == 1);
	CHECK(amuletBonus(ItemKind::StrengthGrand).might == 6);
	CHECK(amuletBonus(ItemKind::ArmorNormal).armor == 4);
	CHECK(amuletBonus(ItemKind::HealthGrand).maxHpPercent == 30);
	CHECK(amuletBonus(ItemKind::PoisonWardNormal).poisonResistPercent == 50);
	CHECK(amuletBonus(ItemKind::TrapWardGrand).trapCutPercent == 100);
	CHECK(amuletBonus(ItemKind::RegenerationNormal).regenHpPerSecond == 1);
	CHECK(amuletBonus(ItemKind::RegenerationGrand).regenHpPerSecond == 2);
	CHECK(amuletBonus(ItemKind::VenomLesser).venomPercent == 10);
	CHECK(amuletBonus(ItemKind::VenomLesser).venomTier == PoisonTier::Weak);
	CHECK(amuletBonus(ItemKind::VenomNormal).venomTier == PoisonTier::Medium);
	CHECK(amuletBonus(ItemKind::VenomGrand).venomPercent == 30);
	CHECK(amuletBonus(ItemKind::VenomGrand).venomTier == PoisonTier::Strong);
	CHECK(amuletBonus(ItemKind::StrengthGrand).venomPercent == 0);
	const Resistances pierce = amuletBonus(ItemKind::PierceWardGrand).resist;
	CHECK(pierce[static_cast<size_t>(DamageType::Pierce)] == 60);
	CHECK(pierce[static_cast<size_t>(DamageType::Blunt)] == NORMAL);
	CHECK(amuletBonus(ItemKind::BluntWardLesser).resist[static_cast<size_t>(DamageType::Blunt)] == 92);
}

TEST_CASE("an amulet is put on, swapped and taken off; it stays in the bag") {
	ItemBag bag;
	Vitals player{true, 10, 50, 100, 100};
	CHECK(bag.Block(ItemKind::StrengthLesser, player) == UseBlock::NotFound);
	bag.Add(ItemKind::StrengthLesser);
	bag.Add(ItemKind::ArmorMinor);
	CHECK(bag.AnyFound(ItemGroup::Amulets));
	CHECK(bag.Use(ItemKind::StrengthLesser, player));
	CHECK(bag.Worn() == ItemKind::StrengthLesser);
	CHECK(bag.Equipped() == ItemKind::Club); // the weapon stays in hand
	CHECK(bag.Use(ItemKind::ArmorMinor, player));
	CHECK(bag.Worn() == ItemKind::ArmorMinor);
	CHECK(bag.Use(ItemKind::ArmorMinor, player)); // again: off
	CHECK_FALSE(bag.Worn().has_value());
	CHECK(bag.Count(ItemKind::ArmorMinor) == 1);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::ArmorMinor, true));
	CHECK(bag.Block(ItemKind::ArmorMinor, Vitals{false, 0, 50, 0, 100}) == UseBlock::Dead);
}

TEST_CASE("the worn amulet survives a save and a load") {
	ItemBag bag;
	bag.Add(ItemKind::HealthGrand);
	bag.Add(ItemKind::HealthGrand);
	CHECK(bag.Use(ItemKind::HealthGrand, Vitals{true, 1, 1, 1, 1}));
	std::stringstream file;
	bag.Save(file);
	ItemBag loaded;
	loaded.Load(file);
	CHECK(loaded.Count(ItemKind::HealthGrand) == 2);
	CHECK(loaded.Worn() == ItemKind::HealthGrand);
}

TEST_CASE("a save from before the amulets: 21 slots in ItemKind order, nothing worn") {
	std::stringstream file;
	file << "INV3 21 ";
	for (int pass = 0; pass < 3; pass++) // counts, levels, found: one of each
		for (int i = 0; i < 21; i++)
			file << 1 << " ";
	file << "2 1\n"; // the composite bow in hand
	ItemBag bag;
	bag.Load(file);
	CHECK(bag.Count(ItemKind::Antidote) == 1);
	CHECK(bag.Count(ItemKind::Javelin) == 1);
	CHECK(bag.Count(ItemKind::StrengthLesser) == 0);
	CHECK(bag.Equipped() == ItemKind::CompositeBow);
	CHECK_FALSE(bag.Worn().has_value());
}

TEST_CASE("a tab shows the found items first, each part in ItemKind order") {
	ItemBag bag; // the club only
	const std::vector<ItemKind> potions = tabOrder(bag, ItemGroup::Potions);
	REQUIRE(potions.size() == static_cast<size_t>(POTION_KIND_COUNT));
	for (int i = 0; i < POTION_KIND_COUNT; i++) // nothing found: plain ItemKind order
		CHECK(potions[static_cast<size_t>(i)] == itemAt(WEAPON_KIND_COUNT + i));

	bag.Add(ItemKind::Antidote);
	bag.Add(ItemKind::LargeHealth);
	const std::vector<ItemKind> found = tabOrder(bag, ItemGroup::Potions);
	CHECK(found[0] == ItemKind::LargeHealth);
	CHECK(found[1] == ItemKind::Antidote);
	CHECK(found[2] == ItemKind::SmallHealth);
	CHECK(tabPosition(bag, ItemKind::Antidote) == 1);
	CHECK(tabPosition(bag, ItemKind::SmallHealth) == 2);
	CHECK(tabPosition(bag, ItemKind::SmallStamina) > tabPosition(bag, ItemKind::Might));

	CHECK(tabOrder(bag, ItemGroup::Weapons).front() == ItemKind::Club);
	CHECK(tabPosition(bag, ItemKind::Club) == 0);
	CHECK(tabOrder(bag, ItemGroup::Amulets).size() == static_cast<size_t>(AMULET_KIND_COUNT));
}

TEST_CASE("an item used up stays in the found part") {
	ItemBag bag;
	bag.Add(ItemKind::Might);
	const Vitals player{true, 50, 50, 100, 100, false};
	REQUIRE(bag.Use(ItemKind::Might, player));
	CHECK(bag.Count(ItemKind::Might) == 0);
	CHECK(tabPosition(bag, ItemKind::Might) == 0);
}

TEST_CASE("sort by name: A to Z among the found, the rest after in ItemKind order") {
	ItemBag bag; // the club
	bag.Add(ItemKind::Spear);
	bag.Add(ItemKind::Dagger);
	bag.Add(ItemKind::SelfBow);
	const std::vector<ItemKind> order = tabOrder(bag, ItemGroup::Weapons, SortOrder::Name);
	CHECK(order[0] == ItemKind::Club);
	CHECK(order[1] == ItemKind::Dagger);
	CHECK(order[2] == ItemKind::SelfBow);
	CHECK(order[3] == ItemKind::Spear);
	CHECK(order[4] == ItemKind::ShortSword); // not found: ItemKind order
	CHECK(order[5] == ItemKind::Khopesh);
}

TEST_CASE("sort by strength: weapons by damage now, amulets by tier, strong potions first") {
	ItemBag bag;
	bag.Add(ItemKind::ShortSword);
	bag.Add(ItemKind::Dagger);
	std::vector<ItemKind> weapons = tabOrder(bag, ItemGroup::Weapons, SortOrder::Strength);
	CHECK(weapons[0] == ItemKind::ShortSword); // 35
	CHECK(weapons[1] == ItemKind::Club);	   // 10
	CHECK(weapons[2] == ItemKind::Dagger);	   // 8
	// the dagger levelled up past the club: damage now, not the base
	for (int copies = 0; copies < 30; copies++)
		bag.Add(ItemKind::Dagger);
	while (bag.Upgrade(ItemKind::Dagger, true)) {
	}
	weapons = tabOrder(bag, ItemGroup::Weapons, SortOrder::Strength);
	CHECK(weapons[1] == ItemKind::Dagger);
	CHECK(weapons[2] == ItemKind::Club);

	bag.Add(ItemKind::ArmorLesser);
	bag.Add(ItemKind::StrengthMinor);
	bag.Add(ItemKind::VenomGrand);
	bag.Add(ItemKind::HealthGrand);
	const std::vector<ItemKind> amulets = tabOrder(bag, ItemGroup::Amulets, SortOrder::Strength);
	CHECK(amulets[0] == ItemKind::HealthGrand); // grand first, then ItemKind order
	CHECK(amulets[1] == ItemKind::VenomGrand);
	CHECK(amulets[2] == ItemKind::StrengthMinor);
	CHECK(amulets[3] == ItemKind::ArmorLesser);

	bag.Add(ItemKind::SmallHealth);
	bag.Add(ItemKind::Might);
	bag.Add(ItemKind::GreaterResistance);
	bag.Add(ItemKind::LargeHealth);
	const std::vector<ItemKind> potions = tabOrder(bag, ItemGroup::Potions, SortOrder::Strength);
	CHECK(potions[0] == ItemKind::LargeHealth);
	CHECK(potions[1] == ItemKind::GreaterResistance);
	CHECK(potions[2] == ItemKind::SmallHealth);
	CHECK(potions[3] == ItemKind::Might);
	CHECK_FALSE(bag.Found(potions[4]));
}

TEST_CASE("sort by recent: the last found first, a fresh copy comes up") {
	ItemBag bag; // the club
	bag.Add(ItemKind::Antidote);
	bag.Add(ItemKind::SmallHealth);
	bag.Add(ItemKind::Might);
	std::vector<ItemKind> order = tabOrder(bag, ItemGroup::Potions, SortOrder::Recent);
	CHECK(order[0] == ItemKind::Might);
	CHECK(order[1] == ItemKind::SmallHealth);
	CHECK(order[2] == ItemKind::Antidote);
	bag.Add(ItemKind::Antidote);
	CHECK(tabPosition(bag, ItemKind::Antidote, SortOrder::Recent) == 0);
	CHECK(tabPosition(bag, ItemKind::Antidote, SortOrder::Found) == 2);
	bag.Reset();
	CHECK(bag.Stamp(ItemKind::Antidote) == 0);
}

TEST_CASE("amulet upgrade: each tier step takes its worth from same-type spares") {
	ItemBag bag;
	CHECK(amuletWorth(AmuletTier::Lesser) == 1);
	CHECK(amuletWorth(AmuletTier::Grand) == 8);
	bag.Add(ItemKind::ArmorLesser);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::ArmorLesser, true)); // no spare
	bag.Add(ItemKind::StrengthLesser);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::ArmorLesser, true)); // another type pays nothing
	bag.Add(ItemKind::ArmorLesser);
	CHECK(bag.UpgradePoints(ItemKind::ArmorLesser) == 1);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::ArmorLesser, false)); // dead
	CHECK(bag.Upgrade(ItemKind::ArmorLesser, true));		   // lesser -> minor: + 1 lesser
	CHECK(bag.Count(ItemKind::ArmorLesser) == 0);
	CHECK(bag.Count(ItemKind::ArmorMinor) == 1);
	CHECK(bag.Found(ItemKind::ArmorMinor));

	bag.Add(ItemKind::ArmorLesser);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::ArmorMinor, true)); // 1 of 2 points
	bag.Add(ItemKind::ArmorLesser);
	CHECK(bag.Upgrade(ItemKind::ArmorMinor, true)); // minor -> normal: 2 lessers
	CHECK(bag.Count(ItemKind::ArmorLesser) == 0);
	CHECK(bag.Count(ItemKind::ArmorNormal) == 1);

	// normal -> grand: 1 minor + 2 lessers; the larger spare goes first, the normal spare is not wanted
	bag.Add(ItemKind::ArmorMinor);
	bag.Add(ItemKind::ArmorLesser);
	bag.Add(ItemKind::ArmorLesser);
	bag.Add(ItemKind::ArmorLesser);
	CHECK(bag.UpgradePoints(ItemKind::ArmorNormal) == 5);
	CHECK(bag.Upgrade(ItemKind::ArmorNormal, true));
	CHECK(bag.Count(ItemKind::ArmorGrand) == 1);
	CHECK(bag.Count(ItemKind::ArmorNormal) == 0);
	CHECK(bag.Count(ItemKind::ArmorMinor) == 0);
	CHECK(bag.Count(ItemKind::ArmorLesser) == 1);

	// grand is the top; a grand never pays for a lower one
	bag.Add(ItemKind::ArmorGrand);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::ArmorGrand, true));
	CHECK(bag.UpgradePoints(ItemKind::ArmorLesser) == 0);
}

TEST_CASE("amulet upgrade: largest spares first, the normal one before minors") {
	ItemBag bag;
	bag.Add(ItemKind::HealthNormal);
	bag.Add(ItemKind::HealthNormal);
	bag.Add(ItemKind::HealthMinor);
	bag.Add(ItemKind::HealthMinor);
	CHECK(bag.Upgrade(ItemKind::HealthNormal, true));
	CHECK(bag.Count(ItemKind::HealthNormal) == 0);
	CHECK(bag.Count(ItemKind::HealthMinor) == 2);
	CHECK(bag.Count(ItemKind::HealthGrand) == 1);
}

TEST_CASE("amulet upgrade: the worn one is upgraded and stays worn, never a spare") {
	ItemBag bag;
	const Vitals player{true, 10, 50, 100, 100};
	bag.Add(ItemKind::StrengthLesser);
	bag.Add(ItemKind::StrengthLesser);
	REQUIRE(bag.Use(ItemKind::StrengthLesser, player));
	CHECK(bag.Upgrade(ItemKind::StrengthLesser, true));
	CHECK(bag.Worn() == ItemKind::StrengthMinor);
	CHECK(bag.Count(ItemKind::StrengthLesser) == 0);

	// two minors, one worn: the worn one goes up, the other pays
	bag.Add(ItemKind::StrengthMinor);
	CHECK(bag.UpgradePoints(ItemKind::StrengthMinor) == 2);
	CHECK(bag.Upgrade(ItemKind::StrengthMinor, true));
	CHECK(bag.Worn() == ItemKind::StrengthNormal);
	CHECK(bag.Count(ItemKind::StrengthMinor) == 0);
	CHECK(bag.Count(ItemKind::StrengthNormal) == 1);

	// a worn lesser does not pay for a minor of its type
	bag.Add(ItemKind::StrengthLesser);
	bag.Add(ItemKind::StrengthMinor);
	const Vitals alive{true, 10, 50, 100, 100};
	REQUIRE(bag.Use(ItemKind::StrengthLesser, alive));
	CHECK(bag.UpgradePoints(ItemKind::StrengthMinor) == 0);
	bag.Add(ItemKind::StrengthLesser);
	CHECK(bag.UpgradePoints(ItemKind::StrengthMinor) == 1);
	bag.Add(ItemKind::StrengthLesser);
	REQUIRE(bag.Upgrade(ItemKind::StrengthMinor, true));
	CHECK(bag.Worn() == ItemKind::StrengthLesser);
	CHECK(bag.Count(ItemKind::StrengthLesser) == 1); // the worn one
	CHECK(bag.Count(ItemKind::StrengthNormal) == 2);

	// the same with nothing else: no upgrade
	ItemBag other;
	other.Add(ItemKind::TrapWardLesser);
	other.Add(ItemKind::TrapWardMinor);
	REQUIRE(other.Use(ItemKind::TrapWardLesser, player));
	CHECK(other.UpgradePoints(ItemKind::TrapWardMinor) == 0);
	CHECK_FALSE(other.CanUpgrade(ItemKind::TrapWardMinor, true));
}

TEST_CASE("regeneration: normal + normal -> grand") {
	ItemBag bag;
	CHECK(*nextTier(ItemKind::RegenerationNormal) == ItemKind::RegenerationGrand);
	CHECK_FALSE(nextTier(ItemKind::RegenerationGrand).has_value());
	bag.Add(ItemKind::RegenerationNormal);
	CHECK_FALSE(bag.CanUpgrade(ItemKind::RegenerationNormal, true));
	bag.Add(ItemKind::RegenerationNormal);
	CHECK(bag.Upgrade(ItemKind::RegenerationNormal, true));
	CHECK(bag.Count(ItemKind::RegenerationGrand) == 1);
	CHECK(bag.Count(ItemKind::RegenerationNormal) == 0);
}
