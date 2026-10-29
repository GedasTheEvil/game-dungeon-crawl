// Plan autosave: the danger guard, the world's safe moments (the exit's level start, the slain boss), the deaths
// counter in the save and the leaderboard, and the autosave beside the save slots.
#include "../../external/doctest/doctest.h"
#include "../../src/state/save_slots.h"
#include "../../src/world/leaderboard.h"
#include "../../src/world/run_stats.h"
#include "sim_world.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {
// The autosave events said since the level loaded, in order.
std::vector<AutosaveMoment> autosaves(const SimWorld& world) {
	std::vector<AutosaveMoment> moments;
	for (const WorldEvent& e : world.said)
		if (e.kind == WorldEvent::Kind::Autosave)
			moments.push_back(e.moment);
	return moments;
}
} // namespace

TEST_CASE("the autosave waits while the player is poisoned, burning, held, in the air or travelling") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/interact"));
	world.WaitTicks(30);
	CHECK_FALSE(world.dungeon.PlayerInDanger());

	world.player.stats.poison.Apply(PoisonTier::Weak, false);
	CHECK(world.dungeon.PlayerInDanger());
	world.player.stats.poison.Cure();
	world.player.burning.Ignite(5.0);
	CHECK(world.dungeon.PlayerInDanger());
	world.player.burning.Douse();
	CHECK_FALSE(world.dungeon.PlayerInDanger());

	world.Jump();
	world.WaitTicks(3);
	REQUIRE(world.player.jump.jumping);
	CHECK(world.dungeon.PlayerInDanger());
	world.Wait(1500);
	CHECK_FALSE(world.dungeon.PlayerInDanger());

	REQUIRE(world.WalkTo(9.5f)); // the teleporter
	world.Interact();
	world.WaitTicks(2);
	REQUIRE(world.dungeon.Travelling());
	CHECK(world.dungeon.PlayerInDanger());
	world.WaitTicks(SimWorld::GATE_TRAVEL_TICKS);
	CHECK_FALSE(world.dungeon.PlayerInDanger());
	CHECK(autosaves(world).empty()); // a teleporter is no level start
}

TEST_CASE("held in the giant scorpion's claws is no moment for the autosave") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/giant_scorpion"));
	world.player.god = true;
	world.Tick();
	for (int t = 0; t < 60000 && world.dungeon.PlayerHeldBy() == nullptr; t += UPDATE_TICK_MS)
		world.Tick();
	REQUIRE(world.dungeon.PlayerHeldBy() != nullptr);
	world.player.stats.poison.Cure();
	CHECK(world.dungeon.PlayerInDanger());
}

TEST_CASE("the exit's next level and a slain boss ask for an autosave") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/autosave"));
	const int level = world.dungeon.LevelNumber();
	REQUIRE(world.WalkTo(4.5f));
	world.Interact();
	world.WaitTicks(2 * SimWorld::GATE_TRAVEL_TICKS);
	REQUIRE(world.dungeon.LevelNumber() == level + 1);
	CHECK(autosaves(world) == std::vector<AutosaveMoment>{AutosaveMoment::LevelStart});

	REQUIRE(world.Load("tests/levels/sobek"));
	REQUIRE(world.WalkTo(10.8f));
	for (int t = 0; t < 400 && world.dungeon.Boss() == nullptr; t++)
		world.Tick();
	REQUIRE(world.dungeon.Boss() != nullptr);
	CHECK(autosaves(world).empty());
	world.dungeon.HurtBoss(world.dungeon.BossHealth());
	world.Tick();
	CHECK(autosaves(world) == std::vector<AutosaveMoment>{AutosaveMoment::Boss});
}

TEST_CASE("the deaths ride in the save; a death rewrites only that number; an old save reads none") {
	RunStats stats;
	stats.campaign = true;
	stats.riddles = 3;
	stats.deaths = 2;
	std::stringstream saved;
	stats.Dump(saved);
	saved << "NEXT";
	RunStats loaded;
	loaded.LoadDump(saved);
	CHECK(loaded.deaths == 2);
	CHECK(loaded.riddles == 3);
	std::string next;
	saved >> next;
	CHECK(next == "NEXT");

	std::string text = "1 STATS 5\n" + saved.str();
	const std::string before = text;
	REQUIRE(RunStats::SetDeaths(text, 7));
	CHECK(text.size() == before.size());
	CHECK(text.find("DEATHS 7\nNEXT") != std::string::npos);
	std::string unchanged = text;
	unchanged.replace(unchanged.find("DEATHS 7"), 8, "DEATHS 2");
	CHECK(unchanged == before);

	std::string old = "RUN 1 0 0 0 0 0 0 0 0\nNAMES 42\nJOURNAL\n"; // from before the autosave
	REQUIRE(RunStats::SetDeaths(old, 1));
	CHECK(old == "RUN 1 0 0 0 0 0 0 0 0\nNAMES 42\nDEATHS 1\nJOURNAL\n");
	std::string none = "no run here\n";
	CHECK_FALSE(RunStats::SetDeaths(none, 1));

	std::stringstream oldSave("RUN 1 0 0 0 0 0 0 0 0 0 0 0\nNAMES 42\nCURSES 0");
	RunStats fromOld;
	fromOld.LoadDump(oldSave);
	CHECK(fromOld.deaths == 0);
	oldSave >> next;
	CHECK(next == "CURSES");
}

TEST_CASE("the run's deaths go on the leaderboard; an old record has none") {
	PlayerStats stats;
	stats.run.deaths = 4;
	RunRecord r = recordOf(stats, "Ahmose", "2026-10-20", "abc");
	CHECK(r.deaths == 4);
	std::stringstream file;
	writeRecord(file, r);
	file << "[run]\nname=Old\n";
	std::vector<std::string> warnings;
	const std::vector<RunRecord> records = readRecords(file, warnings);
	REQUIRE(records.size() == 2);
	CHECK(records[0].deaths == 4);
	CHECK(records[1].deaths == 0);
}

TEST_CASE("the autosave is no save slot: its own file, read with its level") {
	for (int slot = 0; slot < SaveSlots::COUNT; slot++)
		CHECK(SaveSlots::FileName(slot) != SaveSlots::AUTOSAVE);
	const std::string path = "build/autosave_test.sav";
	std::filesystem::remove(path);
	CHECK_FALSE(SaveSlots::DescribeFile(path).used);
	{
		std::ofstream out(path);
		out << "12 STATS";
	}
	const SaveSlots::Info info = SaveSlots::DescribeFile(path);
	CHECK(info.used);
	CHECK(info.level == 12);
	CHECK_FALSE(info.when.empty());
	std::filesystem::remove(path);
}
