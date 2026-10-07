#include "../../external/doctest/doctest.h"
#include "../../src/world/loot.h"

namespace {
OwnedWeapons allWeapons() {
	OwnedWeapons owned{};
	owned.fill(true);
	return owned;
}
} // namespace

TEST_CASE("the same seed rolls the same loot") {
	Rng a(7);
	Rng b(7);
	for (int i = 0; i < 50; i++)
		CHECK(RollChestLoot(ItemKind::ShortSword, allWeapons(), 1, a) ==
			  RollChestLoot(ItemKind::ShortSword, allWeapons(), 1, b));
}

TEST_CASE("a chest holds its item first, then bonuses") {
	Rng rng(3);
	int bonuses = 0;
	for (int i = 0; i < 1000; i++) {
		std::vector<ItemKind> loot = RollChestLoot(ItemKind::LargeHealth, allWeapons(), 1, rng);
		REQUIRE_FALSE(loot.empty());
		CHECK(loot.front() == ItemKind::LargeHealth);
		for (size_t k = 1; k < loot.size(); k++)
			CHECK((isPotion(loot[k]) || isAmulet(loot[k]))); // a potion chest's bonuses are potions, or an amulet
		bonuses += static_cast<int>(loot.size()) - 1;
	}
	// Small stamina 30%, large stamina 20%, the same potion 5%, a small health 10%, an amulet 6%: about 0.71 a chest.
	CHECK(bonuses > 550);
	CHECK(bonuses < 850);
}

TEST_CASE("a weapon chest may add a weaker weapon, never a better one") {
	Rng rng(11);
	for (int i = 0; i < 2000; i++)
		for (ItemKind item : RollChestLoot(ItemKind::Spear, allWeapons(), 1, rng))
			CHECK(item != ItemKind::ShortSword);
}

TEST_CASE("the mimic's chest holds any weapon or potion but the antidote, never an amulet") {
	Rng rng(5);
	bool seen[ITEM_KIND_COUNT] = {};
	for (int i = 0; i < 2000; i++)
		seen[itemIndex(RollMimicLoot(allWeapons(), rng))] = true;
	for (int i = 0; i < ITEM_KIND_COUNT; i++)
		CHECK(seen[i] == (!isAmulet(itemAt(i)) && itemAt(i) != ItemKind::Antidote));
}

TEST_CASE("the bonus weapon and the mimic's weapon are ones the player holds") {
	Rng rng(9);
	OwnedWeapons clubOnly{};
	clubOnly[static_cast<size_t>(itemIndex(ItemKind::Club))] = true;
	for (int i = 0; i < 2000; i++) {
		for (ItemKind item : RollChestLoot(ItemKind::ShortSword, clubOnly, 1, rng))
			CHECK((item == ItemKind::ShortSword || item == ItemKind::Club || isPotion(item) || isAmulet(item)));
		const ItemKind mimic = RollMimicLoot(clubOnly, rng);
		CHECK((mimic == ItemKind::Club || isPotion(mimic)));
	}
}

TEST_CASE("chests give lesser amulets, minor ones only deeper down, never regeneration") {
	Rng rng(13);
	int amulets = 0;
	int minors = 0;
	for (int level : {1, 10, 11, 30})
		for (int i = 0; i < 4000; i++) {
			std::optional<ItemKind> amulet = RollChestAmulet(level, rng);
			if (!amulet)
				continue;
			amulets++;
			const Amulet a = amuletOf(*amulet);
			CHECK(a.type != AmuletType::Regeneration);
			CHECK((a.tier == AmuletTier::Lesser || a.tier == AmuletTier::Minor));
			if (a.tier == AmuletTier::Minor) {
				CHECK(level >= MINOR_AMULET_LEVEL);
				minors++;
			}
		}
	// 6% of 16000 chests: about 960, of them about 40% minor on the two deep levels (about 190).
	CHECK(amulets > 800);
	CHECK(amulets < 1120);
	CHECK(minors > 130);
	CHECK(minors < 260);
}

TEST_CASE("an amulet chest holds only its amulet") {
	Rng rng(17);
	for (int i = 0; i < 200; i++)
		CHECK(RollChestLoot(ItemKind::HealthMinor, allWeapons(), 20, rng) ==
			  std::vector<ItemKind>{ItemKind::HealthMinor});
}

TEST_CASE("a boss leaves a normal or grand amulet, grand more often deeper down") {
	Rng rng(19);
	int grand5 = 0;
	int grand30 = 0;
	for (int i = 0; i < 2000; i++)
		for (int level : {5, 30}) {
			std::optional<ItemKind> drop = RollKillDrop(true, allWeapons(), level, rng);
			REQUIRE(drop);
			REQUIRE(isAmulet(*drop));
			const AmuletTier tier = amuletOf(*drop).tier;
			CHECK((tier == AmuletTier::Normal || tier == AmuletTier::Grand));
			if (tier == AmuletTier::Grand)
				(level == 5 ? grand5 : grand30)++;
		}
	// 10% on level 5, 60% on level 30.
	CHECK(grand5 > 140);
	CHECK(grand5 < 260);
	CHECK(grand30 > 1100);
	CHECK(grand30 < 1300);
}
