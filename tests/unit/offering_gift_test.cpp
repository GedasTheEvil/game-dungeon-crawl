// Plan offering-gift: a shrine god sometimes answers an offering with a gift.
#include "../../external/doctest/doctest.h"
#include "../../src/level/offering_gift.h"
#include "../../src/level/player_stats.h"
#include "../../src/level/run_stats.h"
#include <sstream>

namespace {
constexpr int ROLLS = 20000;

GiftKind kindOf(const OfferingGift& gift) {
	if (gift.item)
		return GiftKind::Item;
	switch (gift.boon.kind) {
	case BoonKind::Might:
		return GiftKind::Might;
	case BoonKind::Armor:
		return GiftKind::Armor;
	case BoonKind::Life:
		return GiftKind::Life;
	default:
		return GiftKind::Stamina;
	}
}
} // namespace

TEST_CASE("offering gift: each god's weights sum to 100") {
	for (int g = 0; g < GOD_COUNT; g++) {
		int sum = 0;
		for (int k = 0; k < GIFT_KIND_COUNT; k++)
			sum += giftWeight(static_cast<God>(g), static_cast<GiftKind>(k));
		CHECK(sum == 100);
	}
}

TEST_CASE("offering gift: about one offering in ten brings one") {
	int gifts = 0;
	for (int i = 0; i < ROLLS; i++)
		gifts += RollOfferingGift(God::Montu, 5, 1234U, i).Any() ? 1 : 0;
	CHECK(gifts > ROLLS * (OFFERING_GIFT_PERCENT - 1) / 100);
	CHECK(gifts < ROLLS * (OFFERING_GIFT_PERCENT + 1) / 100);
}

TEST_CASE("offering gift: the same run and offering roll the same gift, a quick load cannot reroll it") {
	for (int i = 0; i < 200; i++) {
		const OfferingGift a = RollOfferingGift(God::Amun, 12, 77U, i);
		const OfferingGift b = RollOfferingGift(God::Amun, 12, 77U, i);
		CHECK(a.boon == b.boon);
		CHECK(a.item == b.item);
	}
	int differ = 0; // another run rolls other gifts
	for (int i = 0; i < 200; i++)
		differ += RollOfferingGift(God::Amun, 12, 77U, i, true).boon !=
						  RollOfferingGift(God::Amun, 12, 78U, i, true).boon
					  ? 1
					  : 0;
	CHECK(differ > 50);
}

TEST_CASE("offering gift: each god gives every kind, by its weights") {
	for (int g = 0; g < GOD_COUNT; g++) {
		const God god = static_cast<God>(g);
		std::array<int, GIFT_KIND_COUNT> count{};
		for (int i = 0; i < ROLLS; i++) {
			const OfferingGift gift = RollOfferingGift(god, 20, 5U, i, true);
			REQUIRE(gift.Any());
			count[static_cast<size_t>(kindOf(gift))]++;
		}
		for (int k = 0; k < GIFT_KIND_COUNT; k++) {
			const int expected = ROLLS * giftWeight(god, static_cast<GiftKind>(k)) / 100;
			CHECK(count[static_cast<size_t>(k)] > expected * 8 / 10);
			CHECK(count[static_cast<size_t>(k)] < expected * 12 / 10);
		}
	}
}

TEST_CASE("offering gift: the stats are half a boon, the item a treasure chest's for the depth") {
	bool potion = false;
	bool weapon = false;
	for (int i = 0; i < 2000; i++) {
		const OfferingGift gift = RollOfferingGift(God::Amun, 1, 9U, i, true);
		if (gift.item) {
			CHECK_FALSE(gift.boon.Any());
			CHECK((isPotion(*gift.item) || isWeapon(*gift.item)));
			if (isWeapon(*gift.item)) // level 1: only the weapons levelgen puts in from the start
				CHECK(weaponDef(*gift.item).generatedFrom <= 1);
			potion = potion || isPotion(*gift.item);
			weapon = weapon || isWeapon(*gift.item);
			continue;
		}
		const Boon boon = gift.boon;
		CHECK(
			(boon == Boon{BoonKind::Might, 1} || boon == Boon{BoonKind::Armor, 1} || boon == Boon{BoonKind::Life, 5} ||
			 boon == Boon{BoonKind::Stamina, 5})
		);
	}
	CHECK(potion);
	CHECK(weapon);
}

TEST_CASE("offering gift: the offerings counted are kept by a save, an old save has none") {
	RunStats run;
	run.offerings = 13;
	std::stringstream file;
	run.Dump(file);
	file << "NEXT\n";
	RunStats loaded;
	loaded.LoadDump(file);
	CHECK(loaded.offerings == 13);
	std::string next;
	file >> next;
	CHECK(next == "NEXT");

	std::string old = file.str();
	old.erase(old.find("OFFERINGS"), std::string("OFFERINGS 13\n").size());
	std::stringstream oldFile(old);
	RunStats fromOld;
	fromOld.LoadDump(oldFile);
	CHECK(fromOld.offerings == 0);
	oldFile >> next;
	CHECK(next == "NEXT");
}
