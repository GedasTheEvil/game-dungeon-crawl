// Plan save-load-safety: a failed save keeps the old file, a broken save leaves the game as it was, the slots' run
// details.
#include "../../external/doctest/doctest.h"
#include "../../src/level/save_slots.h"
#include "../../src/sim/dungeon_save.h"
#include "sim_world.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {
std::string saved(SimWorld& world) {
	std::ostringstream out;
	UiViewState view;
	writeSaveGame(out, SaveParts{world.dungeon, world.player, world.items, world.journal, view});
	return out.str();
}

std::string fileText(const std::string& path) {
	std::string text;
	REQUIRE(readWholeFile(path, text));
	return text;
}
} // namespace

TEST_CASE("a save goes through a temp file: a failed write reports it and keeps the old file") {
	const std::string dir = "build/save_load_safety";
	std::filesystem::remove_all(dir);
	std::filesystem::create_directories(dir);
	const std::string path = dir + "/save0.sav";

	CHECK(writeFileSafely(path, "first"));
	CHECK(fileText(path) == "first");
	CHECK(writeFileSafely(path, "second"));
	CHECK(fileText(path) == "second");
	CHECK_FALSE(std::filesystem::exists(path + ".tmp"));

	std::filesystem::create_directory(path + ".tmp"); // the temp file cannot be written
	CHECK_FALSE(writeFileSafely(path, "third"));
	CHECK(fileText(path) == "second");
	CHECK_FALSE(writeFileSafely(dir + "/missing/save1.sav", "x"));

	std::string text;
	CHECK_FALSE(readWholeFile(dir + "/none.sav", text));
}

TEST_CASE("a broken save leaves the game as it was; a good one loads") {
	SimWorld world; // a rat at col 3, a chest at col 6
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/win_run"));
	world.WaitTicks(30);
	UiViewState view;
	const SaveParts parts{world.dungeon, world.player, world.items, world.journal, view};
	const std::string good = saved(world);

	world.player.stats.run.AddPlayMs(5000, world.dungeon.LevelNumber());
	REQUIRE(world.WalkTo(2.2f));
	const std::string now = saved(world);
	REQUIRE(now != good);

	CHECK_FALSE(loadSaveGame("", parts));
	CHECK(saved(world) == now);
	CHECK_FALSE(loadSaveGame("not a save", parts));
	CHECK(saved(world) == now);
	CHECK_FALSE(loadSaveGame(good.substr(0, good.size() / 2), parts)); // cut in the map
	CHECK(saved(world) == now);

	const std::string cutEnd = good.substr(0, good.rfind(SaveSlots::END_TAG));
	CHECK_FALSE(loadSaveGame(cutEnd, parts)); // all but the end: cut in the journal, perhaps
	CHECK(saved(world) == now);

	CHECK(loadSaveGame(good, parts));
	CHECK(saved(world) == good);
	// A save from before the tags (plan save-load-safety) has no end to check.
	const std::string tag = std::string(SaveSlots::FILE_TAG) + " ";
	REQUIRE(good.starts_with(tag));
	REQUIRE(loadSaveGame(now, parts));
	CHECK(loadSaveGame(cutEnd.substr(tag.size()), parts));
	CHECK(saved(world) == good);
}

// Plan save-bag-parse-crash: a save cut in the player's part, or with an unknown bag tag, is a broken save, not an
// exception that quits the game.
TEST_CASE("a save cut after the stats, or with an unknown bag tag, does not throw and leaves the game as it was") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/win_run"));
	UiViewState view;
	const SaveParts parts{world.dungeon, world.player, world.items, world.journal, view};
	const std::string good = saved(world);

	const size_t offerings = good.find("OFFERINGS ");
	REQUIRE(offerings != std::string::npos);
	const std::string cutStats = good.substr(0, good.find('\n', offerings) + 1);
	CHECK_FALSE(loadSaveGame(cutStats, parts));
	CHECK(saved(world) == good);

	const size_t tag = good.find("INV8");
	REQUIRE(tag != std::string::npos);
	std::string newer = good;
	newer[tag + 3] = '9';
	CHECK_FALSE(loadSaveGame(newer, parts));
	CHECK(saved(world) == good);
}

TEST_CASE("a slot shows the save's level, the character level and the play time") {
	const std::string dir = "build/save_load_safety_slots";
	std::filesystem::remove_all(dir);
	std::filesystem::create_directories(dir);
	const std::string path = dir + "/save0.sav";

	SimWorld world;
	REQUIRE(world.Load(7));
	world.player.stats.run.AddPlayMs(3 * 60 * 60000 + 4 * 60000 + 59000, 7);
	REQUIRE(writeFileSafely(path, saved(world)));
	SaveSlots::Info info = SaveSlots::DescribeFile(path);
	CHECK(info.used);
	CHECK(info.level == 7);
	CHECK(info.characterLevel == world.player.stats.CurrentLevel());
	CHECK(SaveSlots::PlayTime(info.playMs) == "3h 04m");

	REQUIRE(writeFileSafely(path, "3 2")); // a save from before the tags and the run's statistics: no play time
	info = SaveSlots::DescribeFile(path);
	CHECK(info.level == 3);
	CHECK(info.characterLevel == 2);
	CHECK(info.playMs == -1);

	CHECK(SaveSlots::PlayTime(59000) == "0m");
	CHECK(SaveSlots::PlayTime(12 * 60000) == "12m");
	CHECK_FALSE(SaveSlots::DescribeFile(dir + "/none.sav").used);
}

// Plan save-remembers-ui-state: the stamina-skill toggle, the lit arrows and the screens' settings survive a save and
// a load; a save without their lines loads the defaults.
TEST_CASE("a save keeps the toggle, the lit arrows, the facing, the inventory and the journal pages") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/win_run"));
	UiViewState view;
	const SaveParts parts{world.dungeon, world.player, world.items, world.journal, view};

	world.player.staminaSkills = true;
	world.player.fireArrows = 3;
	view.facing = -1;
	view.tab = ItemGroup::Potions;
	view.sort = {SortOrder::Strength, SortOrder::Name, SortOrder::Recent, SortOrder::Found};
	view.journalSection = 4;
	view.spread = {0, 2, 1, 0, 3, 0, 5};
	const UiViewState set = view;
	std::ostringstream out;
	writeSaveGame(out, parts);

	world.player.Reanimate(); // the toggle and the arrows are dropped
	view = UiViewState{};
	REQUIRE(loadSaveGame(out.str(), parts));
	CHECK(world.player.staminaSkills);
	CHECK(world.player.fireArrows == 3);
	CHECK(view == set);

	// A save from before the lines: all of them gone from it.
	const std::string text = out.str();
	const size_t from = text.find("\nSTAMINASKILLS");
	const size_t to = text.find('\n', text.find("\nJOURNAL") + 1);
	REQUIRE(from != std::string::npos);
	REQUIRE(to != std::string::npos);
	const std::string old = text.substr(0, from) + text.substr(to + 1 - 1);
	world.player.staminaSkills = true;
	world.player.fireArrows = 9;
	view = set;
	REQUIRE(loadSaveGame(old, parts));
	CHECK_FALSE(world.player.staminaSkills);
	CHECK(world.player.fireArrows == 0);
	CHECK(view == UiViewState{});
}
