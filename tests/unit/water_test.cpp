// Half water and deep water in the checker (docs/plan/crocodiles-and-flooded-cells.md), on small levels.
#include "../../external/doctest/doctest.h"
#include "../../src/world/level_check.h"
#include "../../src/world/tile_defs.h"
#include <cstring>
#include <string>
#include <vector>

namespace {
// Objects and structure drawn top row first, as in an ASCII level source (tools/level/ascii2level.py); the last row
// is row 0. The structure drawing uses STRUCTURE_GLYPHS, ' ' keeps what the object drawing gave.
LevelGrid drawn(const std::vector<std::string>& objects, const std::vector<std::string>& structure = {}) {
	LevelGrid grid;
	for (size_t i = 0; i < objects.size(); i++) {
		const int row = static_cast<int>(objects.size() - 1 - i);
		for (size_t col = 0; col < objects[i].size(); col++)
			for (const GlyphDef& g : glyphLegend())
				if (g.glyph == objects[i][col])
					grid.set(static_cast<int>(col), row, g.tile);
	}
	for (size_t i = 0; i < structure.size(); i++) {
		const int row = static_cast<int>(structure.size() - 1 - i);
		for (size_t col = 0; col < structure[i].size(); col++) {
			const char* glyph = std::strchr(STRUCTURE_GLYPHS, structure[i][col]);
			if (structure[i][col] == ' ' || glyph == nullptr)
				continue;
			grid.cells[row * LEVEL_WIDTH + static_cast<int>(col)].structure =
				static_cast<Structure>(glyph - STRUCTURE_GLYPHS);
		}
	}
	return grid;
}

bool warns(const LevelReport& r, const std::string& part) {
	for (const std::string& w : r.warnings)
		if (w.find(part) != std::string::npos)
			return true;
	return false;
}
} // namespace

TEST_CASE("half water is walked through") {
	LevelReport r = checkLevel(drawn({"#######", "#S...E#", "#######"}, {"       ", "  ~~~  ", "  ===  "}));
	CHECK(r.valid);
	CHECK(r.warnings.empty());
}

TEST_CASE("deep water blocks like a wall") {
	CHECK_FALSE(checkLevel(drawn({"#######", "#S...E#", "#######"}, {"       ", "   =   ", "   =   "})).valid);
}

TEST_CASE("no jump out of half water: the spikes cannot be jumped from it") {
	// From dry ground the jump clears the spikes; from the water next to them it does not.
	CHECK(checkLevel(drawn({"#######", "#S.^.E#", "#######"})).valid);
	LevelReport fromWater = checkLevel(drawn({"#######", "#S.^.E#", "#######"}, {"       ", " ~~    ", " ==    "}));
	CHECK(fromWater.pathSpikes == 1); // walks through them instead
	CHECK(fromWater.pathJumps == 0);
}

TEST_CASE("water that is not held up, deep water in the open") {
	LevelReport floating = checkLevel(drawn({"#######", "#S...E#", "#.....#", "#######"}, {"", "   ~   ", "", ""}));
	CHECK(warns(floating, "is not on deep water or a wall"));
	LevelReport deep = checkLevel(drawn({"#######", "#S...E#", "#######"}, {"", "", "   =   "}));
	CHECK(warns(deep, "is not under water"));
}

TEST_CASE("a ladder may start in the water, not go down into it") {
	// Rows 3 and 2: a ladder from the water floor (row 1) up to the exit floor.
	const std::vector<std::string> up = {"#######", "#..HE##", "#..H###", "#S.H..#", "#######"};
	LevelReport fromWater = checkLevel(drawn(up, {"", "", "", "  ~~   ", "  ==   "}));
	CHECK(fromWater.valid);
	CHECK_FALSE(warns(fromWater, "goes down into the water"));
	// The ladder ends above the water: climbing down it drops the player in.
	const std::vector<std::string> down = {"#######", "#S.H###", "#..H###", "#....E#", "#######"};
	LevelReport intoWater = checkLevel(drawn(down, {"", "", "", "   ~   ", "   =   "}));
	CHECK(warns(intoWater, "goes down into the water"));
}

TEST_CASE("a crocodile lives in or next to the water") {
	CHECK(warns(checkLevel(drawn({"#######", "#S.C.E#", "#######"})), "not in or next to water"));
	CHECK_FALSE(warns(checkLevel(drawn({"#######", "#S.C.E#", "#######"}, {"", "    ~  ", "    =  "})),
					  "not in or next to water"));
}
