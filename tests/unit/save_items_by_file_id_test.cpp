// Plan save-items-by-file-id: the bag and the potions drunk are saved by item file id, not by ItemKind position, so a
// new ItemKind anywhere in the enum moves no saved item. tests/saves/items_by_file_id.sav was written by the code with
// the ItemKind order of that day: it must keep loading the same items whatever the order is later.
#include "../../external/doctest/doctest.h"
#include "../../src/level/rules/item_bag.h"
#include "../../src/level/rules/run_stats.h"
#include <fstream>
#include <sstream>
#include <string>

TEST_CASE("the fixed save loads the same items whatever the ItemKind order") {
	std::ifstream file("tests/saves/items_by_file_id.sav");
	REQUIRE(file.is_open());
	ItemBag bag;
	bag.Load(file);
	RunStats run;
	run.LoadDump(file);
	REQUIRE(static_cast<bool>(file));

	CHECK(bag.Count(ItemKind::Club) == 2);
	CHECK(bag.Count(ItemKind::SelfBow) == 2);
	CHECK(bag.Level(ItemKind::SelfBow) == 2);
	CHECK(bag.Count(ItemKind::Spear) == 1);
	CHECK(bag.Level(ItemKind::Spear) == 1);
	CHECK(bag.Equipped() == ItemKind::SelfBow);
	CHECK(bag.Count(ItemKind::SmallHealth) == 3);
	CHECK(bag.Count(ItemKind::Antidote) == 1);
	CHECK(bag.Count(ItemKind::StrengthLesser) == 1);
	CHECK(bag.Worn() == ItemKind::StrengthLesser);
	CHECK(bag.Count(ItemKind::GarnetCopper) == 1);
	CHECK(bag.Count(ItemKind::AmethystSkyIron) == 2);
	CHECK(bag.RingOn(Hand::Left) == ItemKind::GarnetCopper);
	CHECK(bag.RingOn(Hand::Right) == ItemKind::AmethystSkyIron);
	CHECK(bag.Stamp(ItemKind::AmethystSkyIron) > 0);

	int held = 0; // nothing else is in the bag
	for (int i = 0; i < ITEM_KIND_COUNT; i++)
		held += bag.Count(itemAt(i));
	CHECK(held == 2 + 2 + 1 + 3 + 1 + 1 + 1 + 2);

	CHECK(run.Drunk(ItemKind::LargeHealth) == 2);
	CHECK(run.Drunk(ItemKind::Antidote) == 1);
	CHECK(run.DrunkTotal() == 3);
}

TEST_CASE("a record of an unknown item is skipped, the rest loads") {
	std::istringstream file("INV9 2 9 99 5 3 1 7 1 2 1 2 1 1 1 0 0 0 0 0 0 0\n");
	ItemBag bag;
	bag.Load(file);
	CHECK(bag.Count(ItemKind::Spear) == 1);
	CHECK(bag.Level(ItemKind::Spear) == 2);
	CHECK(bag.Stamp(ItemKind::Spear) == 1);
}

TEST_CASE("a save with the potions drunk by position (RUN) still loads") {
	std::istringstream file("RUN 1 0 0 0 0 0 0 0 0 0 1 " + std::to_string(itemIndex(ItemKind::SmallHealth)) + " 4 0\n");
	RunStats run;
	run.LoadDump(file);
	CHECK(run.Drunk(ItemKind::SmallHealth) == 4);
}
