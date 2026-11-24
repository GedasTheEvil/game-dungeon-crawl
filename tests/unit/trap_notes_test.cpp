// The traps' field notes (plan journal-trap-notes): the first touch of a floor trap, the first rock or sand fall the
// player sets off, the first jump into ceiling spikes. Each has a text.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/level/ui/journal_text.h"
#include <algorithm>
#include <string>

namespace {
bool noted(const SimWorld& world, FieldNote note) {
	return std::any_of(world.said.begin(), world.said.end(), [note](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Note && e.note == note;
	});
}

// tests/levels/trap_notes, one hall (row 2): floor spikes at col 5, a death trap at 10, a rock fall at 13, a sand fall
// at 17, ceiling spikes over col 21. A god, so the traps do not stop the walk.
constexpr int HALL = 2;

void loadHall(SimWorld& world) {
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/trap_notes"));
}
} // namespace

TEST_CASE("trap notes: floor spikes, once walked into") {
	SimWorld world;
	loadHall(world);
	REQUIRE(world.WalkTo(3.f));
	CHECK_FALSE(noted(world, FieldNote::FloorSpikes));
	REQUIRE(world.WalkTo(5.5f));
	CHECK(noted(world, FieldNote::FloorSpikes));
}

TEST_CASE("trap notes: the death trap too") {
	SimWorld world;
	loadHall(world);
	REQUIRE(world.dungeon.PlacePlayer(8, HALL));
	world.Tick();
	CHECK_FALSE(noted(world, FieldNote::FloorSpikes));
	REQUIRE(world.WalkTo(10.5f));
	CHECK(noted(world, FieldNote::FloorSpikes));
}

TEST_CASE("trap notes: a rock fall and a sand fall the player sets off") {
	SimWorld rock;
	loadHall(rock);
	REQUIRE(rock.dungeon.PlacePlayer(11, HALL));
	rock.Tick();
	CHECK_FALSE(noted(rock, FieldNote::RockFalls));
	REQUIRE(rock.WalkTo(13.5f));
	CHECK(noted(rock, FieldNote::RockFalls));

	SimWorld sand;
	loadHall(sand);
	REQUIRE(sand.dungeon.PlacePlayer(15, HALL));
	sand.Tick();
	CHECK_FALSE(noted(sand, FieldNote::RockFalls));
	REQUIRE(sand.WalkTo(17.5f));
	CHECK(noted(sand, FieldNote::RockFalls));
}

TEST_CASE("trap notes: a jump into ceiling spikes, not a walk under them") {
	SimWorld world;
	loadHall(world);
	REQUIRE(world.dungeon.PlacePlayer(20, HALL));
	REQUIRE(world.WalkTo(22.5f));
	CHECK_FALSE(noted(world, FieldNote::CeilingTraps));
	REQUIRE(world.dungeon.PlacePlayer(21, HALL));
	world.Tick();
	world.Jump();
	world.Wait(1500);
	CHECK(noted(world, FieldNote::CeilingTraps));
}

TEST_CASE("trap notes: the texts") {
	for (const FieldNote n : {FieldNote::FloorSpikes, FieldNote::RockFalls, FieldNote::CeilingTraps}) {
		CHECK(std::string(fieldNoteText(n).title) != "");
		CHECK(std::string(fieldNoteText(n).text) != "");
	}
}
