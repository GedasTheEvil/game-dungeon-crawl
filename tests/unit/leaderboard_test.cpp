// Plan leaderboard: the run's counters (kept by a save and load), the leaderboard file, its orders and scores.
#include "../../external/doctest/doctest.h"
#include "../../src/world/leaderboard.h"
#include "../../src/world/run_stats.h"
#include "sim_world.h"
#include <filesystem>
#include <sstream>

namespace {
RunRecord run(const std::string& name, int playMs, int xp, int inscriptions, int treasures, int riddles) {
	RunRecord r;
	r.name = name;
	r.playMs = playMs;
	r.xp = xp;
	r.inscriptions = inscriptions;
	r.treasures = treasures;
	r.riddles = riddles;
	return r;
}

std::vector<std::string> namesIn(const std::vector<RunRecord>& records, RecordOrder order) {
	std::vector<std::string> names;
	for (int i : rankRecords(records, order))
		names.push_back(records[static_cast<size_t>(i)].name);
	return names;
}
} // namespace

TEST_CASE("a kill, a treasure and the play time count; a load puts the saved counts back") {
	SimWorld world; // a rat at col 3, a chest at col 6
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/win_run"));
	world.WaitTicks(30);
	RunStats& stats = world.player.stats.run;
	stats.AddPlayMs(1000, world.dungeon.LevelNumber());
	REQUIRE(world.WalkTo(2.2f));
	for (int i = 0; i < 20 && world.Nearest() > 0; i++) {
		world.Attack();
		world.Wait(700);
	}
	REQUIRE(world.Nearest() == 0);
	CHECK(stats.Slain(MonsterRat) == 1);
	CHECK(stats.SlainTotal() == 1);
	CHECK(stats.damageDealt > 0);
	CHECK(stats.biggestHit > 0);
	CHECK(stats.biggestHit <= stats.damageDealt);

	world.SaveAndLoad(); // the saved run: one rat, no treasure yet
	REQUIRE(world.WalkTo(6.5f));
	world.OpenChest();
	CHECK(stats.treasures == 1);
	CHECK(world.player.stats.run.Slain(MonsterRat) == 1);
	world.SaveAndLoad();
	world.SaveAndLoad(); // twice: a load does not add up
	CHECK(world.player.stats.run.treasures == 1);
	CHECK(world.player.stats.run.SlainTotal() == 1);
	CHECK(world.player.stats.run.PlayMs() == 1000);
}

TEST_CASE("run counters: potions by kind, play time per level, the save line and back") {
	RunStats stats;
	stats.campaign = true;
	stats.Drink(ItemKind::SmallHealth);
	stats.Drink(ItemKind::SmallHealth);
	stats.Drink(ItemKind::Antidote);
	stats.AddPlayMs(500, 1);
	stats.AddPlayMs(700, 3);
	stats.Hit(4);
	stats.Hit(9);
	stats.riddles = 2;
	stats.poisoned = 4;
	stats.cursed = 5;
	CHECK(stats.DrunkTotal() == 3);
	CHECK(stats.Drunk(ItemKind::SmallHealth) == 2);
	CHECK(stats.LevelMs() == std::vector<int>{500, 0, 700});
	CHECK(stats.PlayMs() == 1200);
	CHECK(stats.damageDealt == 13);
	CHECK(stats.biggestHit == 9);

	std::stringstream saved;
	stats.Dump(saved);
	saved << "NEXT";
	RunStats loaded;
	loaded.LoadDump(saved);
	CHECK(loaded.campaign);
	CHECK(loaded.Drunk(ItemKind::Antidote) == 1);
	CHECK(loaded.LevelMs() == stats.LevelMs());
	CHECK(loaded.riddles == 2);
	CHECK(loaded.biggestHit == 9);
	CHECK(loaded.poisoned == 4);
	CHECK(loaded.cursed == 5);
	std::string next;
	saved >> next;
	CHECK(next == "NEXT");

	std::stringstream none("CURSES 0"); // no RUN line: a fresh run, the stream left as it was
	RunStats fresh;
	fresh.LoadDump(none);
	CHECK_FALSE(fresh.campaign);
	none >> next;
	CHECK(next == "CURSES");
}

TEST_CASE("a record written and read back; a broken line and a record without a name are skipped") {
	RunRecord r = run("Hatshepsut", 3723000, 9100, 4, 7, 2);
	r.date = "2026-10-13";
	r.version = "abc1234";
	r.skills = {"Dodge", "Sharp eye"};
	r.slain = {{"Rat", 12}, {"Mummy", 3}};
	r.potions = {{"Antidote", 1}};
	r.levelMs = {60000, 120000};
	r.damageTaken = 321;
	r.poisoned = 6;
	r.cursed = 2;
	std::stringstream file;
	writeRecord(file, r);
	file << "[run]\nxp=5\n\n"; // no name
	writeRecord(file, run("Thutmose", 1000, 10, 0, 0, 0));
	file << "this line is broken\nxp=lots\n";

	std::vector<std::string> warnings;
	const std::vector<RunRecord> records = readRecords(file, warnings);
	REQUIRE(records.size() == 2);
	const RunRecord& back = records[0];
	CHECK(back.name == "Hatshepsut");
	CHECK(back.date == "2026-10-13");
	CHECK(back.version == "abc1234");
	CHECK(back.playMs == 3723000);
	CHECK(back.xp == 9100);
	CHECK(back.skills == r.skills);
	CHECK(back.slain == r.slain);
	CHECK(back.potions == r.potions);
	CHECK(back.levelMs == r.levelMs);
	CHECK(back.damageTaken == 321);
	CHECK(back.poisoned == 6);
	CHECK(back.cursed == 2);
	CHECK(back.SlainTotal() == 15);
	CHECK(records[1].name == "Thutmose");
	CHECK(records[1].xp == 10); // the broken xp line after it left it as it was
	CHECK(warnings.size() == 3);

	std::stringstream newer("[run]\nname=Later\nfavourite_colour=7\n"); // a statistic this build does not know
	warnings.clear();
	CHECK(readRecords(newer, warnings).size() == 1);
	CHECK(warnings.empty());
}

TEST_CASE("the orders and their scores: time fastest first, XP and exploration highest first, ties the faster") {
	const std::vector<RunRecord> records = {
		run("slow", 5000000, 900, 1, 1, 1), // explored 10 + 10 + 30 = 50
		run("fast", 1000000, 500, 5, 0, 0), // 50
		run("rich", 3000000, 900, 0, 2, 3), // 110
	};
	CHECK(namesIn(records, RecordOrder::Time) == std::vector<std::string>{"fast", "rich", "slow"});
	CHECK(namesIn(records, RecordOrder::Xp) == std::vector<std::string>{"rich", "slow", "fast"});
	CHECK(namesIn(records, RecordOrder::Explored) == std::vector<std::string>{"rich", "fast", "slow"});
	CHECK(explorationScore(records[2]) == 2 * EXPLORE_TREASURE_POINTS + 3 * EXPLORE_RIDDLE_POINTS);
	CHECK(scoreText(records[0], RecordOrder::Time) == "1:23:20");
	CHECK(scoreText(records[0], RecordOrder::Xp) == "900");
	CHECK(scoreText(records[2], RecordOrder::Explored) == "110");
	CHECK(formatPlayTime(59999) == "0:00:59");
	CHECK(formatPlayTime(36000000) == "10:00:00");
}

TEST_CASE("the board file: missing is empty with the placeholder row, a record appended and read again") {
	const std::string path = "build/test_leaderboard/leaderboard.txt";
	std::filesystem::remove_all("build/test_leaderboard");
	Leaderboard board(path);
	board.Load();
	CHECK(board.Records().empty());
	REQUIRE(board.Shown().size() == 1);
	const RunRecord placeholder = board.Shown()[0];
	CHECK(placeholder.placeholder);
	CHECK(placeholder.name == "Spirit of Exploration");
	CHECK(placeholder.playerLevel == 1);
	for (int order = 0; order < RECORD_ORDER_COUNT; order++)
		CHECK(scoreText(placeholder, static_cast<RecordOrder>(order)) == "-");
	CHECK(board.LastName().empty());

	REQUIRE(board.Add(run("Ahmose", 2000, 30, 1, 0, 0)));
	REQUIRE(board.Add(run("Nefertari", 1000, 40, 0, 0, 0)));
	Leaderboard again(path);
	again.Load();
	REQUIRE(again.Records().size() == 2);
	CHECK_FALSE(again.Shown()[0].placeholder);
	CHECK(again.Shown().size() == 2); // the placeholder is gone
	CHECK(again.LastName() == "Nefertari");
}

TEST_CASE("a poisoning that takes and a curse count; a warded poisoning does not") {
	Player player;
	WorldEvents events;
	GameRandom random;
	random.Seed(1);
	player.Poison(PoisonTier::Weak, events, random.gameplay);
	player.stats.LayCurse(Curse::Weakness, CurseGrade::Lesser, events);
	player.stats.LayCurse(Curse::Weakness, CurseGrade::Lesser, events); // laid again
	CHECK(player.stats.run.poisoned == 1);
	CHECK(player.stats.run.cursed == 2);
	player.stats.Wear(amuletBonus(ItemKind::PoisonWardGrand), true);
	REQUIRE(player.stats.PoisonResistPercent() > 0);
	(void)events.Take();
	constexpr int TRIES = 20;
	for (int i = 0; i < TRIES; i++)
		player.Poison(PoisonTier::Weak, events, random.gameplay);
	int warded = 0;
	for (const WorldEvent& e : events.Take())
		warded += e.text == "Your amulet wards off the poison" ? 1 : 0;
	REQUIRE(warded > 0);
	CHECK(player.stats.run.poisoned == 1 + TRIES - warded);
	const RunRecord r = recordOf(player.stats, "Seti", "", "");
	CHECK(r.poisoned == player.stats.run.poisoned);
	CHECK(r.cursed == 2);
}

TEST_CASE("a record of the player at the win: end stats, skills, kills and potions by name") {
	PlayerStats stats;
	stats.run.Slay(MonsterRat);
	stats.run.Slay(MonsterRat);
	stats.run.Drink(ItemKind::Antidote);
	stats.run.AddPlayMs(4000, 1);
	stats.heka.Earn(5);
	REQUIRE(stats.heka.Learn(SkillId::Dodge, true));
	const RunRecord r = recordOf(stats, "Ramesses", "2026-10-13", "v1");
	CHECK(r.name == "Ramesses");
	CHECK(r.playMs == 4000);
	CHECK(r.playerLevel == 1);
	CHECK(r.maxHp == stats.CurrentMaxHP());
	REQUIRE(r.slain.size() == 1);
	CHECK(r.slain[0].first == monsterKind(MonsterRat)->name);
	CHECK(r.slain[0].second == 2);
	REQUIRE(r.potions.size() == 1);
	CHECK(r.potions[0].first == std::string(itemText(ItemKind::Antidote).name));
	CHECK(r.skills == std::vector<std::string>{skillDef(SkillId::Dodge).name});
}
