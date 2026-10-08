// The checker's movement rules against the game's (docs/plan/movement-model.draft.md), on small levels.
#include "../../external/doctest/doctest.h"
#include "../../src/world/level_check.h"
#include "../../src/world/items.h"
#include "../../src/world/tile_defs.h"
#include <string>
#include <vector>

namespace {
// Rows top first in the levelcheck --map legend; the last row is row 0. Cells not drawn stay wall.
LevelGrid drawn(const std::vector<std::string>& rows) {
	LevelGrid grid;
	for (size_t i = 0; i < rows.size(); i++) {
		int row = static_cast<int>(rows.size() - 1 - i);
		for (size_t col = 0; col < rows[i].size(); col++)
			for (const GlyphDef& g : glyphLegend())
				if (g.glyph == rows[i][col])
					grid.set(static_cast<int>(col), row, g.tile);
	}
	return grid;
}

bool finishable(const LevelGrid& grid) { return checkLevel(grid).valid; }

Tile& cell(LevelGrid& grid, int col, int row) { return grid.cells[row * LEVEL_WIDTH + col]; }
} // namespace

TEST_CASE("a lever opens the gates of its colour") {
	LevelGrid grid = drawn({"#########", "#S./.R.E#", "#########"});
	CHECK(finishable(grid));
	CHECK(checkLevel(grid).warnings.empty());
}

TEST_CASE("a lever pulled in the level file opens nothing") {
	LevelGrid grid = drawn({"#########", "#S./.R.E#", "#########"});
	pullLever(cell(grid, 3, 1));
	CHECK_FALSE(finishable(grid));
}

TEST_CASE("a key opens the gates of its colour") {
	CHECK(finishable(drawn({"#########", "#S.r.R.E#", "#########"})));
	CHECK_FALSE(finishable(drawn({"#########", "#S.b.R.E#", "#########"})));
}

TEST_CASE("a gate saved while opening is open") {
	LevelGrid grid = drawn({"#######", "#S.R.E#", "#######"});
	CHECK_FALSE(finishable(grid));
	setGateState(cell(grid, 3, 1), GateState::Opening);
	CHECK(finishable(grid));
	setGateState(cell(grid, 3, 1), GateState::Open);
	CHECK(finishable(grid));
}

TEST_CASE("no key opens a boss gate") {
	LevelGrid grid = drawn({"#########", "#S.r.Z.E#", "#########"});
	cell(grid, 3, 1).attr = BOSS_LOCK; // a key of the boss colour: the checker reports it, and it opens nothing
	LevelReport r = checkLevel(grid);
	CHECK_FALSE(r.valid);
	CHECK(r.path.empty());
}

TEST_CASE("the path jumps over a trap on the floor") {
	for (const char* row : {"#S..^..E#", "#S..X..E#"}) {
		LevelReport r = checkLevel(drawn({"#########", row, "#########"}));
		REQUIRE(r.valid);
		CHECK(r.pathJumps == 1);
		CHECK(r.pathSpikes == 0);
		CHECK(r.pathDeathTraps == 0);
	}
}

TEST_CASE("a rock fall is walked, not jumped") {
	LevelReport r = checkLevel(drawn({"#########", "#S..v..E#", "#########"}));
	REQUIRE(r.valid);
	CHECK(r.pathJumps == 0);
	CHECK(r.pathRockFalls == 1);
}

TEST_CASE("a jump from a ladder's foot") {
	// The ladder foot (col 3) stands on the floor with a gap after it.
	LevelReport r = checkLevel(drawn({"#########", "###H#####", "#S.H.E###", "####.####", "#########"}));
	REQUIRE(r.valid);
	CHECK(r.pathJumps == 1);
}

TEST_CASE("a dart plate on the path wants an antidote in reach") {
	auto has = [](const LevelReport& r, const std::string& text) {
		for (const std::string& w : r.warnings)
			if (w.find(text) != std::string::npos)
				return true;
		return false;
	};
	LevelGrid grid = drawn({"##########", "#S.____.E#", "##########"});
	LevelReport walked = checkLevel(grid);
	REQUIRE(walked.valid);
	CHECK(walked.dartPlates == 4);
	CHECK(walked.pathDartPlates > 0);
	CHECK(has(walked, "dart trap on the path but no antidote"));
	const ItemFileId antidote = fileIdOf(ItemKind::Antidote);
	cell(grid, 2, 1) = Tile{Treasure, antidote.type, antidote.id};
	CHECK(checkLevel(grid).warnings.empty());
}
