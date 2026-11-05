// Sub-cells where the checker and the game disagreed (plan subcell-checker-mismatches): small levels, the checker's
// verdict on each, then levelcheck's --script for it played in the sim, as `make paths` plays the campaign's in the
// game. Levels drawn in the levelcheck --map legend, rows top first, shapes set per cell (bit = column + 4 * row).
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/world/level_check.h"
#include "../../src/world/path_script.h"
#include "../../src/world/tile_defs.h"
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr std::uint16_t QUARTER_STEP = 0x000f;
constexpr std::uint16_t HALF_STEP = 0x00ff;
constexpr std::uint16_t THREE_QUARTERS = 0x0fff;
constexpr std::uint16_t STAIR_L = 0x0137; // the editor's "Stair L": three quarter steps up to the left in one cell
constexpr std::uint16_t CRAWL_SPACE = 0xfff0;
constexpr std::uint16_t CRAWL_SPACE_LEFT_OPEN = 0xeee0; // its left sub-cell column open floor to ceiling

LevelGrid drawn(const std::vector<std::string>& rows, const std::vector<std::array<int, 3>>& shapes = {}) {
	LevelGrid grid;
	for (size_t i = 0; i < rows.size(); i++) {
		const int row = static_cast<int>(rows.size() - 1 - i);
		for (size_t col = 0; col < rows[i].size(); col++)
			for (const GlyphDef& g : glyphLegend())
				if (g.glyph == rows[i][col])
					grid.set(static_cast<int>(col), row, cellWith(g.tile));
	}
	for (const auto& [col, row, shape] : shapes)
		grid.cells[row * LEVEL_WIDTH + col].shape = static_cast<std::uint16_t>(shape);
	return grid;
}

int msOf(const std::string& word) {
	if (word.size() > 2 && word.substr(word.size() - 2) == "ms")
		return std::stoi(word.substr(0, word.size() - 2));
	return std::stoi(word) * UPDATE_TICK_MS; // ticks
}

// Plays the script levelcheck --script writes for the level in the sim, the scenario commands it uses as the game's
// scenario runner does (docs/testing.md). Returns "" once the player stands in the path's last cell, else what failed.
std::string playScript(const LevelGrid& grid, const LevelReport& r) {
	const std::string script = pathScript(grid, r, "test", static_cast<float>(r.start.col));
	SimWorld world;
	world.player.god = true;
	world.dungeon.LoadGrid(grid, "test");
	world.player.Reanimate();
	std::istringstream lines(script);
	for (std::string text; std::getline(lines, text);) {
		std::istringstream words(text);
		std::string cmd, a, b;
		words >> cmd >> a >> b;
		bool ok = true;
		if (cmd.empty() || cmd[0] == '#' || cmd == "seed" || cmd == "god" || cmd == "peace" || cmd == "level" ||
			cmd == "dump")
			continue;
		if (cmd == "interact" || cmd == "expect") // the exit's: the walk ends there
			break;
		if (cmd == "wait")
			world.Wait(msOf(a));
		else if (cmd == "walk" && a == "to")
			ok = world.WalkTo(std::stof(b));
		else if (cmd == "walk" && (a == "up" || a == "down"))
			ok = world.Climb(a == "up" ? std::stof(b) : -std::stof(b));
		else if (cmd == "hold")
			world.HoldWalk(a == "right" ? 1 : -1, msOf(b));
		else if (cmd == "jump")
			world.Jump();
		else if (cmd == "crouch")
			world.player.crouchHeld = a == "on";
		else
			return "unknown command: " + text;
		if (!ok)
			return "failed: " + text + " at x " + std::to_string(world.X()) + ", y " + std::to_string(world.Y());
	}
	const CellPos goal = r.path.back();
	const auto col = static_cast<int>(std::floor(world.X()));
	const auto row = static_cast<int>(std::floor(world.Y()));
	if (col != goal.col || row != goal.row)
		return "ended in (" + std::to_string(col) + ", " + std::to_string(row) + "), not in the goal cell\n" + script;
	return "";
}

// The path cell's jumps (the moves into it and inside it).
const std::vector<PathTakeOff>& jumpsInto(const LevelReport& r, int col, int row) {
	for (size_t i = 0; i < r.path.size(); i++)
		if (r.path[i].col == col && r.path[i].row == row)
			return r.pathTakeOffs[i];
	static const std::vector<PathTakeOff> NONE_FOUND;
	return NONE_FOUND;
}
} // namespace

TEST_CASE("a step five sub-cells off, over spikes: the script takes off where the checker does") {
	const LevelGrid grid = drawn({"##########", "#S..^.E..#", "##########"}, {{5, 1, QUARTER_STEP}});
	const LevelReport r = checkLevel(grid);
	REQUIRE(r.valid);
	const std::vector<PathTakeOff>& jumps = jumpsInto(r, 5, 1);
	REQUIRE(jumps.size() == 1);
	CHECK(jumps[0].x == 3 * SUBCELLS + SUBCELLS - 1); // the last sub-cell before the spikes
	CHECK(jumps[0].dir == 1);
	CHECK(jumps[0].face == 5);
	const std::string played = playScript(grid, r);
	CHECK_MESSAGE(played.empty(), played);
}

TEST_CASE("after a drop down a shaft the jump goes the path's way, not the way the player walked in") {
	// Walked right to the shaft at col 5, down it, then a jump back left over the spikes onto the step.
	const LevelGrid grid = drawn(
		{"##########", "#S....####", "#####.####", "#E..^.####", "##########"},
		{{3, 1, QUARTER_STEP}}
	);
	const LevelReport r = checkLevel(grid);
	REQUIRE(r.valid);
	const std::vector<PathTakeOff>& jumps = jumpsInto(r, 3, 1);
	REQUIRE(jumps.size() == 1);
	CHECK(jumps[0].dir == -1);
	const std::string played = playScript(grid, r);
	CHECK_MESSAGE(played.empty(), played);
}

TEST_CASE("a jump across a key gate's cell onto a step: the script opens the gate first") {
	// The red gate at col 5 has no floor (a pit under it): the way on is the jump from col 4 across it.
	const LevelGrid grid = drawn(
		{"##########", "#S.r.R.E.#", "#####.####", "##########"},
		{{6, 2, QUARTER_STEP}, {7, 2, QUARTER_STEP}}
	);
	const LevelReport r = checkLevel(grid);
	REQUIRE(r.valid);
	REQUIRE(jumpsInto(r, 6, 2).size() == 1);
	const std::string played = playScript(grid, r);
	CHECK_MESSAGE(played.empty(), played);
}

TEST_CASE("off a ladder onto a half step beside it: the climb to its height, then the step") {
	const LevelGrid grid = drawn(
		{"##########", "###H######", "#S.H...E.#", "##########"},
		{{4, 1, HALF_STEP}, {5, 1, HALF_STEP}}
	);
	const LevelReport r = checkLevel(grid);
	REQUIRE(r.valid);
	// The game: from the ladder's foot the step's face stops the walk.
	SimWorld world;
	world.player.god = true;
	world.dungeon.LoadGrid(grid, "test");
	world.player.Reanimate();
	REQUIRE(world.WalkTo(3.45f));
	CHECK_FALSE(world.WalkTo(4.5f));
	const std::string played = playScript(grid, r);
	CHECK_MESSAGE(played.empty(), played);
}

TEST_CASE("a crawl space right beside a ladder cannot be entered from it; a sub-cell of head room between is enough") {
	// The game: in a ladder's cell the down key climbs and does not crouch, and the body meets the crawl space's
	// ceiling while the player's middle is still in the ladder's cell.
	const std::vector<std::string> rows = {"##########", "##H#######", "#SH...E..#", "##########"};
	const LevelGrid beside = drawn(rows, {{3, 1, CRAWL_SPACE}, {4, 1, CRAWL_SPACE}});
	CHECK_FALSE(checkLevel(beside).valid);
	SimWorld world;
	world.player.god = true;
	world.dungeon.LoadGrid(beside, "test");
	world.player.Reanimate();
	CHECK(world.player.HalfWidth() < SUBCELL_SIZE);
	REQUIRE(world.WalkTo(2.5f)); // into the ladder's cell standing, as down its rungs
	world.player.crouchHeld = true;
	CHECK_FALSE(world.WalkTo(4.5f));
	CHECK_FALSE(world.player.crouched);
	CHECK(world.X() < 3.f); // the middle still in the ladder's cell

	const LevelGrid apart = drawn(rows, {{3, 1, CRAWL_SPACE_LEFT_OPEN}, {4, 1, CRAWL_SPACE}});
	const LevelReport r = checkLevel(apart);
	REQUIRE(r.valid);
	CHECK(r.pathCrawls > 0);
	CHECK(playScript(apart, r).empty());
}

TEST_CASE("a stair in one cell: a jump per step, not one for the cell") {
	const LevelGrid grid = drawn(
		{"##########", "#........#", "#.E....S.#", "##########"},
		{{5, 1, STAIR_L}, {4, 1, THREE_QUARTERS}, {3, 1, THREE_QUARTERS}}
	);
	const LevelReport r = checkLevel(grid);
	REQUIRE(r.valid);
	CHECK(r.pathSteps == 3);
	const std::vector<PathTakeOff>& jumps = jumpsInto(r, 5, 1);
	REQUIRE(jumps.size() == 3);
	for (const PathTakeOff& t : jumps)
		CHECK(t.dir == -1);
	// The stair's foot is a sub-cell in from the cell's side; then a step up per sub-cell.
	CHECK(jumps[0].face == 2);
	CHECK(jumps[1].face == 1);
	CHECK(jumps[2].face == 1);
	const std::string played = playScript(grid, r);
	CHECK_MESSAGE(played.empty(), played);
}
