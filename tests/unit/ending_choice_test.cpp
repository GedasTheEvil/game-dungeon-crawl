// Plan ending-choice: the ankh asks first; Renounced breaks it and writes its own epilogue; the Hall of Records keeps
// the ending (a record from before it reads as Immortal).
#include "../../external/doctest/doctest.h"
#include "../../src/level/journal_text.h"
#include "../../src/level/leaderboard.h"
#include "sim_world.h"
#include <algorithm>
#include <sstream>

namespace {
bool asked(const SimWorld& world) {
	return std::ranges::any_of(world.said, [](const WorldEvent& e) { return e.kind == WorldEvent::Kind::AskEnding; });
}

bool played(const SimWorld& world, WorldSound sound) {
	return std::ranges::any_of(world.said, [sound](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Sound && e.sound == sound;
	});
}

// lvl54, beside the ankh, which was used: the choice asked, nothing won yet.
void atTheAnkh(SimWorld& world) {
	world.player.god = true;
	REQUIRE(world.Load(54));
	REQUIRE(world.dungeon.PlacePlayer(24, 2));
	world.WaitTicks(10);
	REQUIRE(world.WalkTo(26.5f));
	REQUIRE(world.dungeon.Cell(26, 2).type == Ankh);
	world.Interact();
	world.WaitTicks(1);
}
} // namespace

TEST_CASE("ending-choice: the ankh asks first; nothing is won until a pick") {
	SimWorld world;
	atTheAnkh(world);
	CHECK(asked(world));
	CHECK_FALSE(world.dungeon.Won());
	CHECK(world.dungeon.Cell(26, 2).type == Ankh);
}

TEST_CASE("ending-choice: Take the ankh wins Immortal, the ankh stays") {
	SimWorld world;
	atTheAnkh(world);
	world.dungeon.Win(RunEnding::Immortal);
	CHECK(world.dungeon.Won());
	CHECK(world.player.stats.run.ending == RunEnding::Immortal);
	CHECK(world.dungeon.Cell(26, 2).type == Ankh);
	CHECK_FALSE(played(world, WorldSound::JarSmash));
}

TEST_CASE("ending-choice: Renounce wins too, breaks the ankh, and the pick is final") {
	SimWorld world;
	atTheAnkh(world);
	world.dungeon.Win(RunEnding::Renounced);
	world.WaitTicks(1);
	CHECK(world.dungeon.Won());
	CHECK(world.player.stats.run.ending == RunEnding::Renounced);
	CHECK(world.dungeon.Cell(26, 2).type != Ankh);
	CHECK(played(world, WorldSound::JarSmash));
	CHECK(world.Told("ankh cracks"));
	world.dungeon.Win(RunEnding::Immortal); // no second pick
	CHECK(world.player.stats.run.ending == RunEnding::Renounced);
	world.said.clear();
	world.Interact(); // nothing left to use, nothing asked
	world.WaitTicks(1);
	CHECK_FALSE(asked(world));
}

TEST_CASE("ending-choice: the renounced epilogue replaces Dawn; the scales and D. J. still follow") {
	Journal journal;
	journal.Begin();
	journal.LearnNote(FieldNote::HeartLight);
	REQUIRE(journal.FindPage(EXPEDITION_LAST_PAGE, 49));
	journal.WriteEpilogue(RunEnding::Renounced);
	const std::vector<DiaryPage> after(journal.Diary().begin() + 2, journal.Diary().end()); // the prologue's two first
	CHECK(after == std::vector<DiaryPage>{DiaryPage::DawnRenounced, DiaryPage::ScalesLight, DiaryPage::Answer});
	CHECK(std::string(diaryText(DiaryPage::DawnRenounced).title) == "The twelfth hour, refused");
	CHECK(diaryText(DiaryPage::DawnRenounced).paragraphs[2] != nullptr);

	CHECK(static_cast<int>(DiaryPage::DawnRenounced) + 1 == DIARY_PAGE_COUNT); // saved by number, at the end
	std::stringstream save; // the page survives a save
	journal.Dump(save);
	Journal loaded;
	loaded.Load(save);
	CHECK(loaded.Diary() == journal.Diary());
}

TEST_CASE("ending-choice: the record keeps the ending; one from before it reads as Immortal; a bad one is skipped") {
	for (const RunEnding ending : {RunEnding::Immortal, RunEnding::Renounced}) {
		RunRecord r;
		r.name = "Ada";
		r.ending = ending;
		std::stringstream file;
		writeRecord(file, r);
		std::vector<std::string> warnings;
		const std::vector<RunRecord> read = readRecords(file, warnings);
		REQUIRE(read.size() == 1);
		CHECK(read[0].ending == ending);
		CHECK(warnings.empty());
	}
	std::stringstream old("[run]\nname=Old\ntime_ms=1000\n");
	std::vector<std::string> warnings;
	const std::vector<RunRecord> read = readRecords(old, warnings);
	REQUIRE(read.size() == 1);
	CHECK(read[0].ending == RunEnding::Immortal);
	CHECK(warnings.empty());

	std::stringstream bad("[run]\nname=Bad\nending=ascended\n");
	(void)readRecords(bad, warnings);
	CHECK(warnings.size() == 1);
}

TEST_CASE("ending-choice: the record of the player at the win takes the run's ending") {
	PlayerStats stats;
	stats.run.ending = RunEnding::Renounced;
	CHECK(recordOf(stats, "Ada", "2026-11-17", "x").ending == RunEnding::Renounced);
	CHECK(std::string(endingName(RunEnding::Renounced)) == "Renounced the gods");
}
