// The checker on sub-cells (plan subcell-checker): its walker on steps, through crawl spaces and over gaps from a
// step, monsters cut off by a shape, and the shape warnings. Small levels drawn in the levelcheck --map legend, shapes
// set on them (bit = column + 4 * row, row 0 at the bottom).
#include "../../external/doctest/doctest.h"
#include "../../src/world/decor.h"
#include "../../src/world/level_check.h"
#include "../../src/world/tile_defs.h"
#include <algorithm>
#include <string>
#include <vector>

namespace {
// Rows top first; the last row is row 0. Cells not drawn stay wall.
LevelGrid drawn(const std::vector<std::string>& rows) {
	LevelGrid grid;
	for (size_t i = 0; i < rows.size(); i++) {
		const int row = static_cast<int>(rows.size() - 1 - i);
		for (size_t col = 0; col < rows[i].size(); col++)
			for (const GlyphDef& g : glyphLegend())
				if (g.glyph == rows[i][col])
					grid.set(static_cast<int>(col), row, cellWith(g.tile));
	}
	return grid;
}

Tile& cell(LevelGrid& grid, int col, int row) { return grid.cells[row * LEVEL_WIDTH + col]; }

LevelGrid shaped(const std::vector<std::string>& rows, const std::vector<std::pair<int, std::uint16_t>>& shapes) {
	LevelGrid grid = drawn(rows);
	for (const auto& [col, shape] : shapes)
		cell(grid, col, 1).shape = shape;
	return grid;
}

constexpr std::uint16_t QUARTER_STEP = 0x000f;
constexpr std::uint16_t HALF_STEP = 0x00ff;
constexpr std::uint16_t THREE_QUARTERS = 0x0fff;
constexpr std::uint16_t STAIR_UP = 0x08ce; // up to the right, a quarter per sub-cell column
constexpr std::uint16_t CRAWL_SPACE = 0xfff0; // open a quarter high at the floor

const std::vector<std::string> HALL = {"#########", "#S.....E#", "#########"};
// Two cells high: head room over a step three quarters up, and for a jump's whole arc off a step.
const std::vector<std::string> TALL_HALL = {"#########", "#.......#", "#S.....E#", "#########"};

bool warned(const LevelReport& r, const std::string& part) {
	return std::any_of(r.warnings.begin(), r.warnings.end(), [&](const std::string& w) {
		return w.find(part) != std::string::npos;
	});
}
} // namespace

TEST_CASE("a hall with no shapes walks as before: no step, no crawl") {
	const LevelReport r = checkLevel(drawn(HALL));
	REQUIRE(r.valid);
	CHECK(r.pathLength == 6);
	CHECK(r.pathSteps == 0);
	CHECK(r.pathCrawls == 0);
}

TEST_CASE("a jump gets onto a quarter step, walking off it is a drop") {
	const LevelReport r = checkLevel(shaped(HALL, {{3, QUARTER_STEP}}));
	REQUIRE(r.valid);
	CHECK(r.warnings.empty());
	CHECK(r.pathSteps == 1); // a running jump from col 1 lands on it
	CHECK(r.pathJumps == 0);
	CHECK(r.pathDrops == 1);
	CHECK(r.pathLength == 5); // the cells, not the sub-cells
}

TEST_CASE("a half step on the only way: no jump gets onto it") {
	const LevelReport r = checkLevel(shaped(HALL, {{3, HALF_STEP}}));
	CHECK_FALSE(r.valid);
}

TEST_CASE("a stair goes up a quarter at a time, onto a ledge three quarters up") {
	const LevelReport r = checkLevel(shaped(TALL_HALL, {{3, STAIR_UP}, {4, THREE_QUARTERS}}));
	// Under a ceiling a cell up there is no room to stand three quarters up.
	CHECK_FALSE(checkLevel(shaped(HALL, {{3, STAIR_UP}, {4, THREE_QUARTERS}})).valid);
	REQUIRE(r.valid);
	CHECK(r.warnings.empty());
	CHECK(r.pathSteps == 3);
}

TEST_CASE("a half step off the way that nothing gets onto is warned about") {
	const LevelReport r = checkLevel(shaped({"#########", "#S.E...##", "#########"}, {{6, HALF_STEP}}));
	REQUIRE(r.valid);
	CHECK(warned(r, "the step at (col 6, row 1) is out of reach: 2 quarters"));
	// A quarter step there is reached: no warning.
	CHECK(checkLevel(shaped({"#########", "#S.E...##", "#########"}, {{6, QUARTER_STEP}})).warnings.empty());
}

TEST_CASE("a crawl space is crawled through, crouched") {
	const LevelReport r = checkLevel(shaped(HALL, {{3, CRAWL_SPACE}, {4, CRAWL_SPACE}}));
	REQUIRE(r.valid);
	CHECK(r.warnings.empty());
	CHECK(r.pathCrawls == 8);
	CHECK(std::count(r.pathMoves.begin(), r.pathMoves.end(), PathMove::Crawl) == 2);
}

TEST_CASE("a gap jumped from a step's height, onto the floor beyond it") {
	// The step (col 2) is the only floor before the pit (col 3): the jump takes off a quarter up.
	LevelGrid grid = drawn({"########", "#....###", "#S..E###", "###.####", "###.####", "########"});
	cell(grid, 2, 3).shape = QUARTER_STEP;
	const LevelReport r = checkLevel(grid);
	REQUIRE(r.valid);
	CHECK(r.pathJumps == 1);
	CHECK(r.pathSteps == 1);
}

TEST_CASE("a quarter step past water is jumped onto from the dry floor, not out of the water") {
	LevelGrid grid = drawn({"#########", "#S.....E#", "###.#####", "#########"});
	cell(grid, 3, 2).structure = Structure::HalfWater;
	cell(grid, 3, 1).structure = Structure::DeepWater;
	cell(grid, 4, 2).shape = QUARTER_STEP;
	const LevelReport r = checkLevel(grid);
	REQUIRE(r.valid);
	CHECK(r.pathSteps == 1);
	// With water on both sides of the step's near cell, no dry floor is near enough: no way on.
	cell(grid, 2, 2).structure = Structure::HalfWater;
	cell(grid, 2, 1) = cell(grid, 3, 1);
	CHECK_FALSE(checkLevel(grid).valid);
}

TEST_CASE("a large monster does not reach the path through a crawl space, a small one does") {
	for (const char* row : {"#S.E.n#", "#S.E.t#"}) {
		const std::vector<std::string> rows = {"#######", row, "#######"};
		CHECK(checkLevel(drawn(rows)).pathMonsters == 1);
		const LevelReport r = checkLevel(shaped(rows, {{4, CRAWL_SPACE}}));
		CHECK(r.pathMonsters == (row[5] == 't' ? 1 : 0));
	}
}

TEST_CASE("a crawl space that ends at a drop or a trap is warned about") {
	LevelGrid pit = drawn({"#########", "#S.E....#", "#######.#", "#########"});
	cell(pit, 6, 2).shape = CRAWL_SPACE;
	CHECK(warned(checkLevel(pit), "the crawl space at (col 6, row 2) ends at a drop"));
	LevelGrid spikes = shaped({"#########", "#S.E..^.#", "#########"}, {{5, CRAWL_SPACE}});
	CHECK(warned(checkLevel(spikes), "the crawl space at (col 5, row 1) ends at Spike"));
}

TEST_CASE("a ladder, gate or door in a shaped cell, a chest without room to stand, a shape in rock") {
	LevelGrid grid = drawn({"#########", "#S.$.E..#", "#########"});
	cell(grid, 3, 1).shape = QUARTER_STEP; // a chest on a step: room to stand
	CHECK(checkLevel(grid).warnings.empty());
	cell(grid, 3, 1).shape = CRAWL_SPACE;
	CHECK(warned(checkLevel(grid), "at (col 3, row 1) without room to stand at it"));
	cell(grid, 3, 1).shape = NO_SHAPE;
	cell(grid, 5, 1).shape = 0xf000; // a low ceiling over the exit
	CHECK(warned(checkLevel(grid), "in a shaped cell at (col 5, row 1)"));
	cell(grid, 5, 1).shape = NO_SHAPE;
	cell(grid, 1, 2).shape = QUARTER_STEP; // rock takes no shape
	CHECK(warned(checkLevel(grid), "a shape in"));
}

TEST_CASE("a placed prop fits a flat step, a decal or torch is not hidden by the shape") {
	LevelGrid grid = drawn({"#########", "#S....E.#", "#########"});
	cell(grid, 3, 1).dressing.prop = DECOR_POTTERY;
	cell(grid, 3, 1).shape = QUARTER_STEP;
	CHECK(checkLevel(grid).warnings.empty());
	cell(grid, 3, 1).shape = STAIR_UP;
	CHECK(warned(checkLevel(grid), "does not fit the cell's shape"));
	cell(grid, 3, 1).dressing.prop = PLACED_UNSET;

	REQUIRE(DECAL_DEFS[13].anchor == DecalAnchor::Floor);
	cell(grid, 4, 1).dressing.decal = 13;
	cell(grid, 4, 1).shape = 0xf000; // a ceiling: the floor decal shows
	CHECK_FALSE(warned(checkLevel(grid), "hidden"));
	cell(grid, 4, 1).shape = QUARTER_STEP;
	CHECK(warned(checkLevel(grid), "decal 13 at (col 4, row 1) is hidden by the cell's shape"));
	cell(grid, 4, 1).dressing.decal = PLACED_UNSET;
	cell(grid, 4, 1).dressing.torch = TorchPlacing::Torch;
	CHECK_FALSE(warned(checkLevel(grid), "hidden"));
	cell(grid, 4, 1).shape = THREE_QUARTERS;
	CHECK(warned(checkLevel(grid), "the torch at (col 4, row 1) is hidden by the cell's shape"));
}
