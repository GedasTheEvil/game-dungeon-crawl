// The weighing of the heart (plan weighing-of-the-heart): the scales on lvl54 weigh the run once; the verdict is a
// field note, and the epilogue writes its page after Dawn.
#include "../../external/doctest/doctest.h"
#include "../../src/level/decor.h"
#include "../../src/level/judgment.h"
#include "../../src/level/journal_text.h"
#include "../../src/level/level.h"
#include "../../src/level/map_marks.h"
#include "sim_world.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

namespace {
constexpr int ROW = 2; // tests/levels/interact: the corridor's row
constexpr int SCALES_COL = 4; // a cell with nothing to interact with

bool noted(const SimWorld& world, FieldNote note) {
	return std::ranges::any_of(world.said, [note](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Note && e.note == note;
	});
}
} // namespace

TEST_CASE("weighing-of-the-heart: reading, riddles and secrets make the heart light; deaths and ushabti heavy") {
	RunStats run;
	CHECK(weighHeart(run) == Verdict::Light); // nothing on either pan: level is light
	run.deaths = 4;
	CHECK(weighHeart(run) == Verdict::Heavy);
	run.inscriptions = 12;
	run.riddles = 4;
	CHECK(heartLight(run) == 20);
	CHECK(heartHeavy(run) == 20);
	CHECK(weighHeart(run) == Verdict::Light);
	run.Slay(MonsterUshabti);
	CHECK(weighHeart(run) == Verdict::Heavy);
	run.secrets = 1;
	CHECK(weighHeart(run) == Verdict::Light);
}

TEST_CASE("weighing-of-the-heart: the scales weigh once, the verdict a field note") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/interact"));
	world.WaitTicks(30);
	REQUIRE(world.dungeon.PlaceDecor(SCALES_COL, ROW, DECOR_SCALES));
	world.player.stats.run.deaths = 3; // and nothing read: heavy
	REQUIRE(world.WalkTo(SCALES_COL + 0.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Weigh);
	CHECK(std::string(interactionVerb(Interaction::Weigh)) == "Be weighed");
	world.Interact();
	world.WaitTicks(1);
	CHECK(noted(world, FieldNote::HeartHeavy));
	CHECK_FALSE(noted(world, FieldNote::HeartLight));
	CHECK(world.Told("sinks"));
	world.journal.LearnNote(FieldNote::HeartHeavy); // as the game does with the event
	CHECK(world.dungeon.InteractVerb() == Interaction::None); // weighed once
	CHECK(propMark(DECOR_SCALES) == MapMark::Scales);
}

TEST_CASE("weighing-of-the-heart: a light heart rises") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/interact"));
	world.WaitTicks(30);
	REQUIRE(world.dungeon.PlaceDecor(SCALES_COL, ROW, DECOR_SCALES));
	world.player.stats.run.inscriptions = 40;
	REQUIRE(world.WalkTo(SCALES_COL + 0.5f));
	world.Interact();
	world.WaitTicks(1);
	CHECK(noted(world, FieldNote::HeartLight));
	CHECK(world.Told("rises"));
}

TEST_CASE("weighing-of-the-heart: the epilogue answers the verdict after Dawn, before D. J.") {
	for (const Verdict v : {Verdict::Light, Verdict::Heavy}) {
		Journal journal;
		journal.Begin();
		journal.LearnNote(verdictNote(v));
		journal.FindPage(EXPEDITION_LAST_PAGE, 50);
		journal.WriteEpilogue(RunEnding::Immortal);
		const std::vector<DiaryPage>& diary = journal.Diary();
		const auto dawn = std::ranges::find(diary, DiaryPage::Dawn);
		REQUIRE(dawn != diary.end());
		REQUIRE(dawn + 1 != diary.end());
		CHECK(*(dawn + 1) == verdictPage(v));
		REQUIRE(dawn + 2 != diary.end());
		CHECK(*(dawn + 2) == DiaryPage::Answer);
		CHECK(std::string(diaryText(verdictPage(v)).title) == "The scales");
		CHECK(std::string(fieldNoteText(verdictNote(v)).title) == "The weighing of the heart");
	}
	Journal unweighed; // the scales skipped: today's epilogue
	unweighed.Begin();
	unweighed.WriteEpilogue(RunEnding::Immortal);
	CHECK(std::ranges::none_of(unweighed.Diary(), [](DiaryPage p) {
		return p == DiaryPage::ScalesLight || p == DiaryPage::ScalesHeavy;
	}));
}

TEST_CASE("weighing-of-the-heart: the scales stand on lvl54 between the boss gate and the ankh") {
	std::ifstream in("content/levels/lvl54");
	REQUIRE(in.good());
	std::stringstream text;
	text << in.rdbuf();
	CHECK(text.str().contains("\n25 2 32 0\n"));
	CHECK(std::string(DECOR_NAMES[DECOR_SCALES]) == "scales");
	CHECK(decorGlyph(DECOR_SCALES) == 'W');
}

TEST_CASE("weighing-of-the-heart: the verdict notes and pages are saved by number, at the end") {
	CHECK(static_cast<int>(FieldNote::HeartHeavy) + 1 < FIELD_NOTE_COUNT); // the later notes after them (bleeding)
	CHECK(static_cast<int>(DiaryPage::ScalesHeavy) + 1 < DIARY_PAGE_COUNT); // a later page after them (ending-choice)
}

TEST_CASE("weighing-of-the-heart: on lvl54 the scales weigh once on the way, then the ankh wins") {
	SimWorld world; // the behaviour checks of tests/scenarios/solved/weighing_of_the_heart.txt
	world.player.god = true;
	REQUIRE(world.Load(54));
	REQUIRE(world.dungeon.PlacePlayer(24, 2));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(25.5f));
	world.WaitTicks(10);
	REQUIRE(world.dungeon.InteractVerb() == Interaction::Weigh);
	world.Interact();
	world.WaitTicks(5);
	const bool light = noted(world, FieldNote::HeartLight);
	CHECK(light != noted(world, FieldNote::HeartHeavy)); // one verdict
	world.journal.LearnNote(light ? FieldNote::HeartLight : FieldNote::HeartHeavy); // as the game does with the event
	CHECK(world.dungeon.InteractVerb() == Interaction::None);
	REQUIRE(world.WalkTo(26.5f));
	world.Interact();
	world.WaitTicks(1);
	world.dungeon.Win(RunEnding::Immortal); // the ending screen's pick (plan ending-choice)
	CHECK(world.dungeon.Won());
}
