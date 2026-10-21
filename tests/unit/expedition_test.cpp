// The earlier expedition's notebook pages (plan expedition-notes): picked up on touch into the journal, kept over a
// save, one of each number over the campaign in order of depth, the last one on the exit path.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/decor.h"
#include "../../src/world/journal_text.h"
#include "../../src/world/level_check.h"
#include "sim_world.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <sstream>

TEST_CASE("expedition page: picked up on touch into the journal, the skeleton stays, kept over a save") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/expedition"));
	constexpr int COL = 7;
	const auto row = static_cast<int>(world.Y());
	REQUIRE(world.dungeon.Cell(COL, row).type == Page);
	CHECK(world.dungeon.PropAt(COL, row) == DECOR_SKELETON);
	CHECK(world.journal.Pages().empty());

	REQUIRE(world.WalkTo(COL + 0.5f));
	world.Tick();
	REQUIRE(world.journal.Pages().size() == 1);
	CHECK(world.journal.Pages()[0].number == 1);
	CHECK(world.Told("torn notebook page"));
	CHECK(world.dungeon.Cell(COL, row).type == Page); // taken, the skeleton under it stays
	CHECK(world.dungeon.Cell(COL, row).value == 1);

	world.SaveAndLoad();
	CHECK(world.journal.Pages().size() == 1);
	CHECK(world.dungeon.Cell(COL, row).value == 1);
	CHECK(world.dungeon.PropAt(COL, row) == DECOR_SKELETON);
}

TEST_CASE("expedition pages keep their numbers' order in the journal, each written once, over a save") {
	Journal j;
	CHECK(j.FindPage(5, 9));
	CHECK(j.FindPage(2, 3));
	CHECK_FALSE(j.FindPage(5, 9));
	CHECK_FALSE(j.FindPage(0, 1));
	CHECK_FALSE(j.FindPage(EXPEDITION_PAGE_COUNT + 1, 1));
	REQUIRE(j.Pages().size() == 2);
	CHECK(j.Pages()[0].number == 2);
	CHECK(j.Pages()[1].level == 9);

	std::stringstream save;
	j.Dump(save);
	Journal loaded;
	loaded.Load(save);
	REQUIRE(loaded.Pages().size() == 2);
	CHECK(loaded.Pages()[0].number == 2);
	CHECK(loaded.Pages()[1].number == 5);
	CHECK(loaded.Pages()[1].level == 9);
}

TEST_CASE("every expedition page has a heading and a text") {
	for (int n = 1; n <= EXPEDITION_PAGE_COUNT; n++) {
		CAPTURE(n);
		const ExpeditionPageText& page = expeditionPageText(n);
		CHECK(std::strlen(page.heading) > 0);
		CHECK(std::strlen(page.text) > 0);
	}
}

TEST_CASE("campaign: each expedition page once, in order of depth, all in reach; the last one on the exit path") {
	std::map<int, int> levelOf; // page number -> campaign level
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		LevelGrid grid;
		REQUIRE(loadLevelFile(campaignLevelFile(n).c_str(), grid).empty());
		const LevelReport report = checkLevel(grid);
		CHECK(report.reachablePages == report.pages);
		for (int cell = 0; cell < LEVEL_WIDTH * LEVEL_HEIGHT; cell++) {
			const Tile& t = grid.cells[cell];
			if (t.type != Page)
				continue;
			CAPTURE(t.attr);
			CHECK(isExpeditionPage(t.attr));
			CHECK(levelOf.count(t.attr) == 0);
			levelOf[t.attr] = n;
			if (t.attr != EXPEDITION_PAGE_COUNT)
				continue;
			const bool onPath = std::any_of(report.path.begin(), report.path.end(), [&](const CellPos& p) {
				return p.col == cell % LEVEL_WIDTH && p.row == cell / LEVEL_WIDTH;
			});
			CHECK(onPath);
		}
	}
	CHECK(levelOf.size() == static_cast<size_t>(EXPEDITION_PAGE_COUNT));
	int last = 0;
	for (const auto& [number, level] : levelOf) {
		CAPTURE(number);
		CHECK(level >= last);
		last = level;
	}
}
