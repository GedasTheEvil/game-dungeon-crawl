// Field notes for the riddle gate, the name gate, wading and the dark (plan field-notes-gates-and-water): each is
// written the first time the player meets the rule, and has a text.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/level/decor.h"
#include "../../src/level/journal_text.h"
#include <algorithm>
#include <string>

namespace {
bool noted(const SimWorld& world, FieldNote note) {
	return std::any_of(world.said.begin(), world.said.end(), [note](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Note && e.note == note;
	});
}
} // namespace

TEST_CASE("gate and water notes: the texts, saved by number after the trap notes") {
	CHECK(static_cast<int>(FieldNote::RiddleGates) == static_cast<int>(FieldNote::CeilingTraps) + 1);
	CHECK(static_cast<int>(FieldNote::TheDark) + 1 == static_cast<int>(FieldNote::HeartLight)); // then the verdicts
	for (const FieldNote n : {FieldNote::RiddleGates, FieldNote::NameGates, FieldNote::Wading, FieldNote::TheDark}) {
		CHECK(std::string(fieldNoteText(n).title) != "");
		CHECK(std::string(fieldNoteText(n).text) != "");
	}
}

TEST_CASE("gate and water notes: opening a riddle gate's scroll") {
	SimWorld world; // tests/levels/riddle: gates at cols 4 and 8
	REQUIRE(world.Load("tests/levels/riddle"));
	REQUIRE(world.WalkTo(3.f));
	CHECK_FALSE(noted(world, FieldNote::RiddleGates));
	REQUIRE(world.WalkTo(4.5f));
	CHECK_FALSE(noted(world, FieldNote::RiddleGates)); // standing at the gate is not enough
	world.Interact();
	world.WaitTicks(1);
	CHECK(noted(world, FieldNote::RiddleGates));
}

TEST_CASE("gate and water notes: touching a sealed name gate") {
	SimWorld world; // tests/levels/name_gate: the gate at col 10
	REQUIRE(world.Load("tests/levels/name_gate"));
	REQUIRE(world.WalkTo(10.5f));
	CHECK_FALSE(noted(world, FieldNote::NameGates));
	world.Interact();
	world.WaitTicks(1);
	CHECK(noted(world, FieldNote::NameGates));
	CHECK_FALSE(noted(world, FieldNote::RiddleGates));
}

TEST_CASE("gate and water notes: the first step in half water") {
	SimWorld world; // tests/levels/water: dry floor to col 4, the pool from col 5
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/water"));
	REQUIRE(world.WalkTo(3.f));
	CHECK_FALSE(noted(world, FieldNote::Wading));
	REQUIRE(world.WalkTo(6.5f));
	CHECK(noted(world, FieldNote::Wading));
}
