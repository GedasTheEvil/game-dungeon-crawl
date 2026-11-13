// Pressure plates (plan pressure-plate): while every plate of a colour carries its weight, the silver plate gates of
// that colour stand open; under it they come down, not on anyone in the gate's cell. Weights: the player, a stone, an
// emptied chest, a body, dragged there (plan mirror-drag's drag). A save keeps the stone on its plate. The checker
// warns when nothing on a plate's floor run is heavy enough.
#include "../../external/doctest/doctest.h"
#include "../../src/level/level_check.h"
#include "../../src/level/map_marks.h"
#include "../../src/level/tile_defs.h"
#include "sim_world.h"
#include <cmath>
#include <string>

namespace {
// tests/levels/pressure_plate, row 2: a stone at col 3; the red plate (needs 3) at col 6, its gate at col 9; the blue
// plate (needs 2) at col 11, its gate at col 13; a treasure at col 16; the green plates (2 each) at cols 18 and 20,
// their gate at col 23; a rat at col 26, the gold plate (needs 1) at col 28, its gate at col 31.
constexpr int ROW = 2;

GateState gateAt(const SimWorld& world, int col) { return gateState(world.dungeon.Cell(col, ROW)); }

SimWorld& loaded(SimWorld& world, int col) {
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/pressure_plate"));
	REQUIRE(world.dungeon.PlacePlayer(col, ROW));
	world.Tick();
	return world;
}

// Ticks past a gate's full motion.
void settle(SimWorld& world) { world.Wait(GATE_OPEN_MS + 100); }
} // namespace

TEST_CASE("the player on a plate that needs their weight: its gate opens, off it, it closes again") {
	SimWorld world;
	loaded(world, 10);
	CHECK(gateAt(world, 13) == GateState::Closed);
	REQUIRE(world.WalkTo(11.5f));
	CHECK(world.dungeon.WeightOn(11, ROW) == PLAYER_WEIGHT);
	world.Tick();
	CHECK(world.dungeon.PlateDown(11, ROW));
	CHECK(gateAt(world, 13) == GateState::Opening);
	settle(world);
	CHECK(gateAt(world, 13) == GateState::Open);
	CHECK(world.Told("silver gate rises"));

	REQUIRE(world.WalkTo(10.3f)); // off the plate
	world.Tick();
	CHECK(gateAt(world, 13) == GateState::Closing);
	world.Wait(GATE_OPEN_MS / 2);
	REQUIRE(world.WalkTo(11.5f)); // back on half way down: it goes up again from there
	world.Tick();
	CHECK(gateAt(world, 13) == GateState::Opening);
	settle(world);
	CHECK(gateAt(world, 13) == GateState::Open);
}

TEST_CASE("the player alone is too light for the red plate; the stone dragged onto it holds it down for good") {
	SimWorld world;
	loaded(world, 5);
	REQUIRE(world.WalkTo(6.5f));
	settle(world);
	CHECK(world.dungeon.WeightOn(6, ROW) == PLAYER_WEIGHT);
	CHECK_FALSE(world.dungeon.PlateDown(6, ROW));
	CHECK(gateAt(world, 9) == GateState::Closed);

	REQUIRE(world.WalkTo(4.5f));
	REQUIRE(world.dungeon.ToggleDrag(-1)); // the stone at col 3
	world.Tick();
	CHECK(world.Told("You take hold of the stone"));
	REQUIRE(world.WalkTo(6.75f)); // pulled along: it lies at col 6, on the plate
	CHECK(world.dungeon.Cell(6, ROW).type == Stone);
	world.dungeon.LetGo();
	REQUIRE(world.WalkTo(8.5f)); // off it, in front of the gate
	settle(world);
	CHECK(world.dungeon.WeightOn(6, ROW) == STONE_WEIGHT);
	CHECK(gateAt(world, 9) == GateState::Open);
	REQUIRE(world.WalkTo(10.5f)); // through

	world.SaveAndLoad(); // the stone stays on the plate, the gate stays up
	world.Tick();
	CHECK(world.dungeon.Cell(6, ROW).type == Stone);
	CHECK(gateAt(world, 9) != GateState::Closed);
	settle(world);
	CHECK(gateAt(world, 9) == GateState::Open);
}

TEST_CASE("a plate gate does not come down on the player standing in its cell") {
	SimWorld world;
	loaded(world, 10);
	REQUIRE(world.WalkTo(11.5f));
	settle(world);
	REQUIRE(gateAt(world, 13) == GateState::Open);
	world.dungeon.PlacePlayer(13, ROW); // under the gate, off the plate
	world.Wait(500);
	CHECK(gateAt(world, 13) == GateState::Open);
	REQUIRE(world.WalkTo(14.6f));
	world.Wait(100);
	CHECK(gateAt(world, 13) != GateState::Open);
}

TEST_CASE("both green plates must be held; an emptied chest dragged onto one weighs") {
	SimWorld world;
	loaded(world, 16);
	REQUIRE(world.dungeon.PlaceContainer(16, ROW, static_cast<int>(Container::Chest), 0));
	world.OpenChest(); // the potion taken: the open chest stays, emptied
	REQUIRE(world.dungeon.ContainerEmptied(16, ROW));
	REQUIRE(world.WalkTo(15.5f));
	REQUIRE(world.dungeon.ToggleDrag(1));
	world.Tick();
	CHECK(world.Told("You take hold of the chest"));
	REQUIRE(world.WalkTo(17.75f)); // pushed: it lies at col 18, on the first plate
	CHECK(world.dungeon.ContainerEmptied(18, ROW));
	CHECK_FALSE(world.dungeon.ContainerEmptied(16, ROW));
	world.dungeon.LetGo();
	settle(world);
	CHECK(world.dungeon.WeightOn(18, ROW) == CHEST_WEIGHT);
	CHECK(world.dungeon.PlateDown(18, ROW));
	CHECK(gateAt(world, 23) == GateState::Closed); // the second plate is still up

	REQUIRE(world.WalkTo(20.5f)); // the player on the second
	settle(world);
	CHECK(gateAt(world, 23) == GateState::Open);
}

TEST_CASE("a rat's body dragged onto the gold plate holds it") {
	SimWorld world;
	loaded(world, 24);
	world.Tick(); // the rat came out as its cell came into view
	auto* monsters = const_cast<Monster*>(world.dungeon.Monsters());
	Monster* rat = nullptr;
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (monsters[i].Active() && monsters[i].Col() == 26)
			rat = &monsters[i];
	REQUIRE(rat != nullptr);
	rat->TrapHit(10000);
	world.Wait(200);
	REQUIRE_FALSE(rat->Alive());
	REQUIRE(world.dungeon.PlacePlayer(25, ROW));
	world.Tick();
	REQUIRE(world.dungeon.ToggleDrag(1));
	world.Tick();
	CHECK(world.Told("You take hold of the body"));
	REQUIRE(world.WalkTo(28.2f)); // pushed, it slides with the player
	CHECK(rat->CentreX() == doctest::Approx(world.X() + DRAG_HOLD));
	world.dungeon.LetGo();
	REQUIRE(world.WalkTo(26.5f));
	settle(world);
	CHECK(std::floor(rat->CentreX()) == 28.f);
	CHECK(world.dungeon.WeightOn(28, ROW) >= 1);
	CHECK(gateAt(world, 31) == GateState::Open);
}

TEST_CASE("the checker: a plate gate opens once its plate is reached; a plate nothing heavy can reach warns") {
	LevelGrid grid;
	REQUIRE(loadLevelFile("tests/levels/pressure_plate", grid).empty());
	const LevelReport r = checkLevel(grid);
	CHECK(r.errors.empty());
	CHECK(r.valid);
	bool blueWarned = false, otherWarned = false;
	for (const std::string& w : r.warnings) {
		if (w.find("pressure plate") == std::string::npos && w.find("plate gate") == std::string::npos)
			continue;
		(w.find("blue pressure plate at (col 11, row 2) needs 2") != std::string::npos ? blueWarned
																					   : otherWarned) = true;
	}
	CHECK(blueWarned); // only the player stands there
	CHECK_FALSE(otherWarned); // the stone, the treasure, the rat's body

	grid.cells[static_cast<size_t>(ROW * LEVEL_WIDTH + 31)] = Tile{}; // the gold gate gone: a plate without one
	bool goldWarned = false;
	for (const std::string& w : checkLevel(grid).warnings)
		goldWarned = goldWarned || w == "a gold pressure plate, but no plate gate of it";
	CHECK(goldWarned);
}

TEST_CASE("plate gates: glyphs, colours, map marks") {
	CHECK(isGateColour(plateGateColour(4)));
	CHECK(plateLockOf(plateGateColour(2)) == 2);
	CHECK_FALSE(isPlateGateColour(BOSS_LOCK));
	CHECK(tileGlyph({Gate, plateGateColour(3), 0}) == '3');
	CHECK(thingMark(Tile{Gate, plateGateColour(2), 0}) == MapMark::Gate);
	CHECK(markSprite({MapMark::Gate, Tile{Gate, plateGateColour(2), 0}}).lock == 2);
	CHECK(thingMark(fixtureAsTile(Fixture{PressurePlate, 1, 3})) == MapMark::PressurePlate);
}
