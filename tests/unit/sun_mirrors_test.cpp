// Sunlight mirrors (plan sunlight-mirrors): the beam's way through the mirrors, the checker's search over the mirrors
// the player can turn, and the beam in the world: a turned mirror, a lit sun disc opening its gate, a burning mummy.
#include "../../external/doctest/doctest.h"
#include "../../src/world/level_check.h"
#include "../../src/world/sun_beam.h"
#include "sim_world.h"
#include <algorithm>
#include <string>
#include <vector>

namespace {
// tests/levels/mirrors: the light shaft at (3, 3) over the mirror at (3, 2), pointing left; the red sun disc at
// (8, 2), the red gate at (10, 2), the exit at (12, 2).
constexpr int SHAFT = 3 * LEVEL_WIDTH + 3;
constexpr int MIRROR = 2 * LEVEL_WIDTH + 3;
constexpr int DISC = 2 * LEVEL_WIDTH + 8;
constexpr int GATE = 2 * LEVEL_WIDTH + 10;

LevelGrid mirrorsLevel() {
	LevelGrid grid;
	REQUIRE(loadLevelFile("tests/levels/mirrors", grid).empty());
	return grid;
}

bool warns(const LevelReport& r, const std::string& text) {
	for (const std::string& w : r.warnings)
		if (w.find(text) != std::string::npos)
			return true;
	return false;
}

bool lit(const std::vector<BeamStep>& steps, int cell) {
	for (const BeamStep& s : steps)
		if (s.cell == cell)
			return true;
	return false;
}

auto gameBlock(const LevelGrid& grid) {
	return [&grid](int cell) { return blocksBeam(grid.cells[cell]); };
}
} // namespace

TEST_CASE("the beam runs down from the light shaft, the mirror sends it its way; edge-on it passes, facing it it "
		  "stops") {
	LevelGrid grid = mirrorsLevel();
	std::vector<BeamStep> steps = traceBeams(grid.cells, gameBlock(grid));
	REQUIRE(steps.size() == 4); // the shaft, the mirror, two cells to the west wall
	CHECK(steps[0].cell == SHAFT);
	CHECK(steps[0].out == BeamDir::Down);
	CHECK(steps[1].cell == MIRROR);
	CHECK(steps[1].in == BeamDir::Down);
	CHECK(steps[1].out == BeamDir::Left);
	CHECK(steps[3].cell == 2 * LEVEL_WIDTH + 1);
	CHECK(steps[3].ends);
	CHECK_FALSE(lit(steps, DISC));

	turnMirror(grid.cells[MIRROR]); // up: back at the shaft, it faces the beam and catches it
	CHECK(mirrorDir(grid.cells[MIRROR]) == BeamDir::Up);
	steps = traceBeams(grid.cells, gameBlock(grid));
	REQUIRE(steps.size() == 2);
	CHECK(steps[1].ends);
	CHECK(steps[1].out == BeamDir::Up);

	turnMirror(grid.cells[MIRROR]); // right: along the row to the closed gate
	steps = traceBeams(grid.cells, gameBlock(grid));
	CHECK(lit(steps, DISC));
	CHECK(steps.back().cell == GATE - 1);
	CHECK(steps.back().ends);
	setGateState(grid.cells[GATE], GateState::Open);
	steps = traceBeams(grid.cells, gameBlock(grid));
	CHECK(lit(steps, GATE + 2)); // on through the open gate to the exit

	turnMirror(grid.cells[MIRROR]); // down: edge-on, the beam goes by into the floor
	steps = traceBeams(grid.cells, gameBlock(grid));
	REQUIRE(steps.size() == 2);
	CHECK(steps[1].in == BeamDir::Down);
	CHECK(steps[1].out == BeamDir::Down);
}

TEST_CASE("the mirrors the player can turn: the beam can light the sun disc, as the mirror stands it cannot") {
	const LevelGrid grid = mirrorsLevel();
	std::vector<std::vector<MirrorSetting>> ways;
	const std::vector<char> any = sunlitCells(grid.cells, gameBlock(grid), [](int) { return true; }, &ways);
	CHECK(any[DISC] != 0);
	REQUIRE(ways[DISC].size() == 1);
	CHECK(ways[DISC][0].cell == MIRROR);
	CHECK(ways[DISC][0].dir == BeamDir::Right);
	CHECK(any[GATE] == 0); // the gate stays closed in this search
	const std::vector<char> fixed = sunlitCells(grid.cells, gameBlock(grid), [](int) { return false; });
	CHECK(fixed[DISC] == 0);
	CHECK(fixed[MIRROR - 1] != 0);
}

TEST_CASE("the checker: turning the mirror lights the disc and opens the gate; unreachable, the level fails") {
	LevelGrid grid = mirrorsLevel();
	LevelReport r = checkLevel(grid);
	CHECK(r.valid);
	CHECK(r.warnings.empty());
	CHECK(r.lightShafts == 1);
	CHECK(r.mirrors == 1);
	CHECK(r.sunDiscs == 1);
	CHECK(r.pathTurns == 1);
	REQUIRE(r.pathTurnWays.size() == 1);
	REQUIRE(r.pathTurnWays[0].size() == 1);
	CHECK(r.pathTurnWays[0][0].dir == BeamDir::Right);

	LevelGrid walled = grid; // a wall between the mirror and an entrance east of it: the mirror stays as it is
	walled.set(4, 2, wallTile());
	walled.set(4, 3, wallTile());
	walled.set(1, 2, Tile{});
	walled.set(5, 2, Tile{Door, GateEntrance, 0});
	r = checkLevel(walled);
	CHECK_FALSE(r.valid);

	LevelGrid preset = grid; // pointing right already: lit as the level starts
	preset.cells[MIRROR].value = static_cast<int>(BeamDir::Right);
	r = checkLevel(preset);
	CHECK(r.valid);
	CHECK(warns(r, "is lit as the level starts"));
	CHECK(r.pathTurns == 0);
}

TEST_CASE("the checker: a light shaft without rock behind it, a disc no beam reaches, a mirror over an inscription") {
	LevelGrid grid = mirrorsLevel();
	grid.set(3, 4, Tile{}); // a hole over the shaft
	CHECK(warns(checkLevel(grid), "has no rock behind it"));

	grid = mirrorsLevel();
	setFixture(grid.cells[DISC], Fixture{});
	setFixture(grid.cells[3 * LEVEL_WIDTH + 9], Fixture{SunDisc, 1, 0}); // up in the air, off every beam's way
	const LevelReport r = checkLevel(grid);
	CHECK(warns(r, "no beam reaches the sun disc"));
	CHECK_FALSE(r.valid);

	grid = mirrorsLevel();
	setFixture(grid.cells[MIRROR], Fixture{Inscription, 1, 0});
	CHECK(warns(checkLevel(grid), "stands in front of an inscription"));
}

TEST_CASE("in the game: the mirror turns with the interact key, the beam lights the sun disc and its gate opens") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/mirrors"));
	world.WaitTicks(10);
	CHECK(world.dungeon.Sunlit(1, 2));
	CHECK_FALSE(world.dungeon.Sunlit(8, 2));
	REQUIRE(world.WalkTo(3.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Turn);
	world.Interact();
	world.WaitTicks(2);
	CHECK(mirrorDir(world.dungeon.Cell(3, 2)) == BeamDir::Up);
	CHECK_FALSE(sunDiscLit(world.dungeon.Cell(8, 2).fixture));
	world.Interact();
	world.WaitTicks(2);
	CHECK(world.dungeon.Sunlit(8, 2));
	CHECK(sunDiscLit(world.dungeon.Cell(8, 2).fixture));
	CHECK(world.Told("The Aten blazes"));
	CHECK(std::any_of(world.said.begin(), world.said.end(), [](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Note && e.note == FieldNote::SunMirrors;
	}));
	world.Wait(GATE_OPEN_MS + 200);
	CHECK(gateState(world.dungeon.Cell(10, 2)) == GateState::Open);
	CHECK(world.dungeon.Sunlit(12, 2));

	world.Interact(); // turned away, the disc stays lit and the gate open
	world.WaitTicks(2);
	CHECK_FALSE(world.dungeon.Sunlit(8, 2));
	CHECK(sunDiscLit(world.dungeon.Cell(8, 2).fixture));
	REQUIRE(world.WalkTo(12.5f));
}

TEST_CASE("a mummy in the beam burns; the beam counts as light for the camel spiders") {
	SimWorld world; // tests/levels/mirrors_mummy: the beam along the row, the mummy at col 9
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/mirrors_mummy"));
	world.WaitTicks(10);
	CHECK(world.dungeon.Lit(6, 2));
	REQUIRE(world.WalkTo(8.f));
	world.Wait(2000);
	const Monster* mummy = nullptr;
	for (int k = 0; k < MAX_MONSTERS; k++)
		if (world.dungeon.Monsters()[k].Active() && world.dungeon.Monsters()[k].Type()->id == MonsterMummy)
			mummy = &world.dungeon.Monsters()[k];
	REQUIRE(mummy != nullptr);
	CHECK(mummy->Burns());
	CHECK(mummy->Health() < mummy->MaxHealth());
}
