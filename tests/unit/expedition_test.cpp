// The earlier expedition's notebook pages (plan expedition-notes): picked up on touch into the journal, kept over a
// save, one of each number over the campaign in order of depth, the last one on the exit path.
#include "../../external/doctest/doctest.h"
#include "../../src/level/campaign.h"
#include "../../src/level/decor.h"
#include "../../src/level/journal_text.h"
#include "../../src/level/level_check.h"
#include "sim_world.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <vector>

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

TEST_CASE("the expedition pages are numbered in order of depth without gaps: Day N rising, the last page last") {
	int day = 0;
	for (int n = 1; n < EXPEDITION_LAST_PAGE; n++) {
		CAPTURE(n);
		const std::string heading = expeditionPageText(n).heading;
		REQUIRE(heading.starts_with("Day "));
		const int number = std::stoi(heading.substr(4));
		CHECK(number > day);
		day = number;
	}
	CHECK(std::string(expeditionPageText(EXPEDITION_LAST_PAGE).heading) == "The last page");
	// Plan expedition-ending: the back cover, written after the last page was torn out.
	CHECK(EXPEDITION_BACK_COVER == EXPEDITION_PAGE_COUNT);
	CHECK(std::string(expeditionPageText(EXPEDITION_BACK_COVER).heading) == "Inside the back cover");
	CHECK(EXPEDITION_PAGE_COUNT >= 18); // one page every two or three levels of the 40 to 60
}

TEST_CASE(
	"campaign: each expedition page once, in order of depth, all in reach; the last one on the exit path, the "
	"back cover off it"
) {
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
			if (t.attr != EXPEDITION_LAST_PAGE && t.attr != EXPEDITION_BACK_COVER)
				continue;
			const bool onPath = std::any_of(report.path.begin(), report.path.end(), [&](const CellPos& p) {
				return p.col == cell % LEVEL_WIDTH && p.row == cell / LEVEL_WIDTH;
			});
			CHECK(
				onPath == (t.attr == EXPEDITION_LAST_PAGE)
			); // the back cover: lvl49's right route (plan expedition-ending)
		}
	}
	// Pages the rebuild has not placed yet (plan campaign-texts) are fine: at most one tile of each number.
	CHECK(levelOf.size() <= static_cast<size_t>(EXPEDITION_PAGE_COUNT));
	int last = 0;
	for (const auto& [number, level] : levelOf) {
		CAPTURE(number);
		CHECK(level >= last);
		last = level;
	}
}

TEST_CASE(
	"the epilogue answers D. J. only when his last page was found; the back cover changes the answer (plan "
	"expedition-ending)"
) {
	Journal none;
	none.WriteEpilogue(RunEnding::Immortal);
	CHECK(none.Diary() == std::vector<DiaryPage>{DiaryPage::Dawn});

	Journal last;
	REQUIRE(last.FindPage(EXPEDITION_LAST_PAGE, 49));
	last.WriteEpilogue(RunEnding::Immortal);
	CHECK(last.Diary() == std::vector<DiaryPage>{DiaryPage::Dawn, DiaryPage::Answer});

	Journal both;
	REQUIRE(both.FindPage(EXPEDITION_LAST_PAGE, 49));
	REQUIRE(both.FindPage(EXPEDITION_BACK_COVER, 49));
	both.WriteEpilogue(RunEnding::Immortal);
	CHECK(both.Diary() == std::vector<DiaryPage>{DiaryPage::Dawn, DiaryPage::AnswerBackCover});
	CHECK(std::string(diaryText(DiaryPage::AnswerBackCover).title) == "For D. J.");
	CHECK(diaryText(DiaryPage::Answer).paragraphs[2] != nullptr);

	std::stringstream save; // the answer page survives a save
	both.Dump(save);
	Journal loaded;
	loaded.Load(save);
	CHECK(loaded.Diary() == both.Diary());
}

TEST_CASE("lvl54: with the last page and the back cover, taking the ankh wins (plan expedition-ending)") {
	SimWorld world; // the behaviour check of tests/scenarios/solved/expedition_ending.txt
	world.player.god = true;
	REQUIRE(world.Load(54));
	REQUIRE(world.dungeon.PlacePlayer(24, 2));
	world.WaitTicks(10);
	REQUIRE(world.journal.FindPage(EXPEDITION_LAST_PAGE, 54));
	REQUIRE(world.journal.FindPage(EXPEDITION_BACK_COVER, 54));
	REQUIRE(world.WalkTo(26.5f));
	CHECK_FALSE(world.dungeon.Won());
	world.Interact();
	world.WaitTicks(1);
	world.dungeon.Win(RunEnding::Immortal); // the ending screen's pick (plan ending-choice)
	CHECK(world.dungeon.Won());
}
