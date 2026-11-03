// The Shrine of Isis (plan shrine-of-isis): praying lifts every curse and starts the Protection of Isis, in which no
// curse lands; it runs out after 2 minutes (4 with the Blessing of Isis skill), ends on death, a prayer again restarts
// it.
#include "../../external/doctest/doctest.h"
#include "../../src/world/decor.h"
#include "sim_world.h"
#include "../../src/world/journal_text.h"
#include <algorithm>
#include <string>

namespace {
constexpr int ROW = 2; // tests/levels/interact: the corridor's row
constexpr int SHRINE_COL = 4; // a cell with nothing to interact with

void curse(Player& player, WorldEvents& events) {
	REQUIRE(player.stats.LayCurse(Curse::Weakness, CurseGrade::Greater, events));
	REQUIRE(player.stats.LayCurse(Curse::Insanity, CurseGrade::Lesser, events));
	REQUIRE(player.stats.curses.Any());
}
} // namespace

TEST_CASE("Shrine of Isis: Pray at the shrine lifts every curse and wards them off") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/interact"));
	world.WaitTicks(30);
	REQUIRE(world.dungeon.PlaceDecor(SHRINE_COL, ROW, DECOR_SHRINE_ISIS));
	curse(world.player, world.events);
	REQUIRE(world.WalkTo(SHRINE_COL + 0.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Pray);
	world.Interact();
	world.WaitTicks(1);
	CHECK_FALSE(world.player.stats.curses.Any());
	CHECK(world.player.stats.IsisWarded());
	CHECK(world.Told("The curses lift"));
	CHECK(std::any_of(world.said.begin(), world.said.end(), [](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Note && e.note == FieldNote::ShrineOfIsis;
	}));
	CHECK(std::any_of(world.said.begin(), world.said.end(), [](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Sound && e.sound == WorldSound::ShrinePray;
	}));
	CHECK(world.dungeon.InteractVerb() == Interaction::Pray); // no cooldown
}

TEST_CASE("Protection of Isis: no curse lands, of any kind or grade, from a monster or a tomb") {
	Player player;
	WorldEvents events;
	player.stats.Pray(events);
	events.Take();
	const int cursed = player.stats.run.cursed;
	for (int c = 0; c < CURSE_COUNT; c++) {
		CHECK_FALSE(player.stats.LayCurse(static_cast<Curse>(c), CurseGrade::Lesser, events));
		CHECK_FALSE(player.stats.LayCurse(static_cast<Curse>(c), CurseGrade::Greater, events));
	}
	CHECK_FALSE(player.stats.curses.Any());
	CHECK(player.stats.run.cursed == cursed); // not counted
	Rng rng(1);
	CurseEntries sure{};
	sure[0] = {Curse::Palsy, CurseGrade::Lesser, 100};
	CHECK_FALSE(player.Curse(sure, events, rng)); // a monster's hit
	CHECK_FALSE(player.stats.curses.Any());
	const std::vector<WorldEvent> said = events.Take();
	REQUIRE_FALSE(said.empty());
	CHECK(said.back().text.find("Isis turns the curse away") != std::string::npos);
}

TEST_CASE("Protection of Isis: lasts 2 minutes, then curses land again; a second prayer clears and restarts") {
	GameClock::enableVirtual();
	Player player;
	WorldEvents events;
	player.stats.Pray(events);
	CHECK(player.stats.IsisWardLeftMs() == ISIS_WARD_MS);
	CHECK(ISIS_WARD_MS == 120000);
	player.stats.AdvanceIsisWard(90000, events);
	CHECK(player.stats.IsisWardLeftMs() == 30000);
	player.stats.AdvanceIsisWard(30000, events);
	CHECK_FALSE(player.stats.IsisWarded());
	const std::vector<WorldEvent> said = events.Take();
	CHECK(std::any_of(said.begin(), said.end(), [](const WorldEvent& e) {
		return e.text.find("Protection of Isis fades") != std::string::npos;
	}));
	curse(player, events);
	player.stats.Pray(events);
	CHECK_FALSE(player.stats.curses.Any());
	CHECK(player.stats.IsisWardLeftMs() == ISIS_WARD_MS);
}

TEST_CASE("Protection of Isis: twice as long with the Blessing of Isis skill") {
	Player player;
	WorldEvents events;
	player.stats.heka.Earn(5);
	REQUIRE(player.stats.heka.Learn(SkillId::BlessingOfIsis, true));
	player.stats.Pray(events);
	CHECK(player.stats.IsisWardLeftMs() == 2 * ISIS_WARD_MS);
}

TEST_CASE("Protection of Isis: ends on death, kept over a save and load") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/interact"));
	world.player.stats.Pray(world.events);
	world.Wait(1000);
	const int left = world.player.stats.IsisWardLeftMs();
	REQUIRE(left > 0);
	world.SaveAndLoad();
	CHECK(world.player.stats.IsisWardLeftMs() == left);
	world.player.TakeHit(100000, DamageMix{100, 0, 0}, world.events, true);
	REQUIRE_FALSE(world.player.Alive());
	CHECK_FALSE(world.player.stats.IsisWarded());
	world.player.Reanimate(); // a new game
	world.player.stats.Pray(world.events);
	world.player.Reanimate();
	CHECK_FALSE(world.player.stats.IsisWarded());
}

TEST_CASE("Shrine of Isis: its field note is the last, saved by number, with a text") {
	CHECK(static_cast<int>(FieldNote::ShrineOfIsis) + 1 == FIELD_NOTE_COUNT);
	CHECK(std::string(fieldNoteText(FieldNote::ShrineOfIsis).title) == "The Shrine of Isis");
	CHECK(std::string(fieldNoteText(FieldNote::ShrineOfIsis).text) != "");
}
