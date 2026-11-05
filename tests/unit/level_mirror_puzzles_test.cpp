// The campaign's later mirror puzzles (plan level-content-gaps): sideways light shafts, one or two mirrors. As the
// level starts no beam falls on the sun disc; turned as the player would (interact: a quarter turn clockwise), it does.
#include "../../external/doctest/doctest.h"
#include "../../src/world/level_check.h"
#include "../../src/world/sun_beam.h"
#include <string>
#include <vector>

namespace {
constexpr int cell(int col, int row) { return row * LEVEL_WIDTH + col; }

LevelGrid campaignLevel(int level) {
	LevelGrid grid;
	const std::string path = "content/levels/lvl" + std::to_string(level);
	REQUIRE(loadLevelFile(path.c_str(), grid).empty());
	return grid;
}

bool discLit(const LevelGrid& grid, int disc) {
	REQUIRE(grid.cells[disc].fixture.type == SunDisc);
	const std::vector<BeamStep> steps = traceBeams(grid.cells, [&grid](int c, BeamDir way) {
		return blocksBeam(grid.cells[c], way);
	});
	for (const BeamStep& s : steps)
		if (s.cell == disc)
			return true;
	return false;
}

void turn(LevelGrid& grid, int mirror, int times) {
	REQUIRE(grid.cells[mirror].type == Mirror);
	for (int k = 0; k < times; k++)
		turnMirror(grid.cells[mirror]);
}
} // namespace

TEST_CASE("lvl35: the shaft in the east wall shines along the hall; one turn of the mirror lights the blue disc") {
	LevelGrid grid = campaignLevel(35);
	CHECK(shaftDir(grid.cells[cell(38, 25)]) == BeamDir::Left);
	CHECK_FALSE(discLit(grid, cell(6, 25)));
	turn(grid, cell(14, 25), 1); // down to left: on along the row
	CHECK(discLit(grid, cell(6, 25)));
}

TEST_CASE("lvl38: the beam needs the mirror by the shaft edge-on, the green gate open and the mirror by the disc") {
	LevelGrid grid = campaignLevel(38);
	const int gate = cell(26, 24);
	REQUIRE(grid.cells[gate].type == Gate);
	turn(grid, cell(12, 24), 1); // up to right: edge-on, the beam goes by
	CHECK_FALSE(discLit(grid, cell(33, 24))); // the green gate stops it
	setGateState(grid.cells[gate], GateState::Open);
	CHECK_FALSE(discLit(grid, cell(33, 24))); // the second mirror sends it into the floor
	turn(grid, cell(31, 24), 3); // down, left, up, right
	CHECK(discLit(grid, cell(33, 24)));
}

TEST_CASE("lvl40: two mirrors, the green disc high on the wall over the second one") {
	LevelGrid grid = campaignLevel(40);
	CHECK(shaftDir(grid.cells[cell(38, 9)]) == BeamDir::Left);
	CHECK_FALSE(discLit(grid, cell(17, 10)));
	turn(grid, cell(17, 9), 2); // up: nothing reaches it yet
	CHECK_FALSE(discLit(grid, cell(17, 10)));
	turn(grid, cell(30, 9), 1); // left: on along the row to the second mirror, up to the disc
	CHECK(discLit(grid, cell(17, 10)));
}
