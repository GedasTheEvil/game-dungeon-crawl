// The looks layer (plan placed-container-looks): the level format v7, the scatter reading it, the stance filter, the
// checker's warnings, the --map drawing, the editor's brushes and the looks placed in the campaign.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/decor_scatter.h"
#include "../../src/world/level_check.h"
#include "../../tools/editor/dressing_brush.h"
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>

namespace {
constexpr int COFFIN = static_cast<int>(Container::Coffin);
constexpr int ROW = 1;
constexpr int FIRST = 2; // the treasures stand from here to LAST
constexpr int LAST = 19;

int idx(int col, int row) { return row * LEVEL_WIDTH + col; }

Dressing& dressing(LevelGrid& grid, int col, int row) { return grid.cells[idx(col, row)].dressing; }

std::unique_ptr<DecorLayout> scatter(const LevelGrid& grid, int depth) {
	auto layout = std::make_unique<DecorLayout>();
	scatterDecor(grid.cells, "looks", depth, grid.meta.zone, *layout);
	return layout;
}

std::string written(const LevelGrid& grid) {
	std::ostringstream out;
	writeLevel(out, grid.cells);
	return out.str();
}

std::string readError(const std::string& text, LevelGrid& loaded, int* version = nullptr) {
	std::istringstream in(text);
	return readLevel(in, loaded.cells, version);
}

bool warns(const LevelReport& r, const std::string& part) {
	for (const std::string& w : r.warnings)
		if (w.find(part) != std::string::npos)
			return true;
	return false;
}

// A corridor on row 1 from col 1 to 20 (rock above and below), entrance at col 1, exit at col 20, a treasure on every
// cell between.
LevelGrid treasureRow() {
	LevelGrid grid;
	for (int col = 1; col <= 20; col++)
		grid.set(col, ROW, Tile{});
	grid.set(1, ROW, Tile{Door, GateEntrance, 0});
	grid.set(20, ROW, Tile{Door, GateExit, 0});
	for (int col = FIRST; col <= LAST; col++)
		grid.set(col, ROW, Tile{Treasure, 1, 0});
	return grid;
}
} // namespace

TEST_CASE("v7 keeps every cell's container look; unset cells are not listed") {
	LevelGrid grid = treasureRow();
	dressing(grid, 3, ROW).container = static_cast<int>(Container::Jar);
	dressing(grid, 4, ROW) = Dressing{};
	dressing(grid, 4, ROW).container = COFFIN;
	dressing(grid, 4, ROW).lid = CoffinLid::Inscribed;
	dressing(grid, 5, ROW).container = COFFIN;
	dressing(grid, 5, ROW).lid = CoffinLid::Plain;
	const std::string file = written(grid);
	CHECK(file.find("\ncontainers 3\n3 1 4 0\n4 1 6 2\n5 1 6 1\n") != std::string::npos);
	LevelGrid loaded;
	int version = 0;
	REQUIRE(readError(file, loaded, &version).empty());
	CHECK(version == LEVEL_VERSION);
	for (int c = 0; c < LEVEL_WIDTH * LEVEL_HEIGHT; c++) {
		CHECK(grid.cells[c].dressing.container == loaded.cells[c].dressing.container);
		CHECK(grid.cells[c].dressing.lid == loaded.cells[c].dressing.lid);
	}
}

TEST_CASE("a v6 level loads with every look unset") {
	LevelGrid grid = treasureRow();
	std::string v6 = written(grid);
	const std::string head = std::string(LEVEL_MAGIC) + " " + std::to_string(LEVEL_VERSION) + " ";
	REQUIRE(v6.rfind(head, 0) == 0);
	v6.replace(0, head.size(), std::string(LEVEL_MAGIC) + " 6 ");
	v6.erase(v6.find("containers "));
	LevelGrid loaded;
	int version = 0;
	REQUIRE(readError(v6, loaded, &version).empty());
	CHECK(version == 6);
	for (const Tile& t : loaded.cells)
		CHECK_FALSE(hasPlacedContainer(t.dressing));
}

TEST_CASE("v7 refuses a look it does not know, and a lid on anything but a coffin") {
	const std::string text = written(treasureRow());
	auto readWith = [&](const std::string& list) {
		std::string bad = text;
		bad.replace(bad.find("containers 0\n"), 13, list);
		LevelGrid loaded;
		return readError(bad, loaded);
	};
	CHECK(readWith("containers 1\n2 1 7 0\n").find("unknown container 7") != std::string::npos);
	CHECK(readWith("containers 1\n2 1 -2 0\n").find("unknown container") != std::string::npos);
	CHECK(readWith("containers 1\n2 1 6 3\n").find("unknown lid") != std::string::npos);
	CHECK(readWith("containers 1\n2 1 0 2\n").find("a lid on a chest") != std::string::npos);
	CHECK(readWith("containers 1\n2 99 0 0\n").find("outside the level") != std::string::npos);
	CHECK(readWith("").find("no looks layer") != std::string::npos);
	CHECK(readWith("containers 1\n2 1 6 2\n").empty());
}

TEST_CASE("a placed look beats the roll of the kind and the lid; the other cells roll as before") {
	constexpr int CAVE = 1; // the cave tier rolls no coffins and no coffers
	LevelGrid grid = treasureRow();
	const auto rolled = scatter(grid, CAVE);
	for (int col = FIRST; col <= LAST; col++)
		REQUIRE(rolled->treasure[idx(col, ROW)].kind != Container::Coffin);

	LevelGrid placed = grid;
	dressing(placed, 3, ROW).container = static_cast<int>(Container::Coffer);
	for (int col = 5; col <= LAST; col++)
		paintLook(dressing(placed, col, ROW), col % 2 == 0 ? INSCRIBED_COFFIN_BRUSH : PLAIN_COFFIN_BRUSH);
	const auto layout = scatter(placed, CAVE);
	CHECK(layout->treasure[idx(3, ROW)].kind == Container::Coffer);
	for (int col : {2, 4}) { // unset: the same look as without the layer
		const TreasureLook& a = rolled->treasure[idx(col, ROW)];
		const TreasureLook& b = layout->treasure[idx(col, ROW)];
		CHECK(a.kind == b.kind);
		CHECK(a.yaw == b.yaw);
		CHECK(a.inscribed == b.inscribed);
	}
	for (int col = 5; col <= LAST; col++) {
		const TreasureLook& look = layout->treasure[idx(col, ROW)];
		CHECK(look.kind == Container::Coffin);
		CHECK(look.inscribed == (col % 2 == 0));
	}
}

TEST_CASE("a coffin with a rolled lid keeps the depth's roll of the text") {
	constexpr int TOMB = 20;
	LevelGrid grid = treasureRow();
	for (int col = FIRST; col <= LAST; col++)
		dressing(grid, col, ROW).container = COFFIN;
	const auto layout = scatter(grid, TOMB);
	int inscribed = 0;
	for (int col = FIRST; col <= LAST; col++)
		inscribed += layout->treasure[idx(col, ROW)].inscribed ? 1 : 0;
	CHECK(inscribed > 0); // 40 % of 18
	CHECK(inscribed < LAST - FIRST + 1);
}

TEST_CASE("a placed coffin takes only the stances facing the camera, by a side wall too") {
	LevelGrid grid = treasureRow();
	// Pockets above the row: a cell with rock on both sides, where the roll favours the stances against a side wall.
	for (int col = 3; col <= 17; col += 2) {
		grid.set(col, ROW + 1, Tile{Treasure, 1, 0});
		paintLook(dressing(grid, col, ROW + 1), COFFIN);
	}
	for (int col = FIRST; col <= LAST; col++)
		paintLook(dressing(grid, col, ROW), COFFIN);
	const auto layout = scatter(grid, 1);
	int sideWall = 0; // the chests' share: some take a side wall stance, the coffins none
	for (int i = 0; i < LEVEL_WIDTH * LEVEL_HEIGHT; i++)
		if (hasPlacedContainer(grid.cells[i].dressing)) {
			CHECK(std::abs(layout->treasure[i].yaw) < 90);
			uint32_t weights[STANCE_COUNT];
			CHECK(containerStances(grid.cells, i % LEVEL_WIDTH, i / LEVEL_WIDTH, Container::Coffin, weights) > 0);
			uint32_t chest[STANCE_COUNT];
			containerStances(grid.cells, i % LEVEL_WIDTH, i / LEVEL_WIDTH, Container::Chest, chest);
			sideWall += chest[6] + chest[7] > 0 ? 1 : 0;
		}
	CHECK(sideWall > 0);
}

TEST_CASE("the checker warns for a look off a treasure and for a plain lid on a trapped coffin") {
	LevelGrid grid = treasureRow();
	paintLook(dressing(grid, 3, ROW), INSCRIBED_COFFIN_BRUSH);
	paintLook(dressing(grid, 4, ROW), static_cast<int>(Container::Table));
	CHECK(checkLevel(grid).warnings.empty());

	LevelGrid empty = grid;
	empty.set(5, ROW, Tile{});
	paintLook(dressing(empty, 5, ROW), static_cast<int>(Container::Jar));
	CHECK(warns(checkLevel(empty), "the jar look at (col 5, row 1) is on a cell without a treasure"));

	LevelGrid trapped = grid;
	trapped.set(6, ROW, Tile{TrappedTreasure, 1, 0});
	paintLook(dressing(trapped, 6, ROW), PLAIN_COFFIN_BRUSH);
	CHECK(warns(checkLevel(trapped), "plain lid on a trapped treasure"));
	paintLook(dressing(trapped, 6, ROW), INSCRIBED_COFFIN_BRUSH);
	CHECK(checkLevel(trapped).warnings.empty());
}

TEST_CASE("levelcheck --map draws the placed looks") {
	LevelGrid grid = treasureRow();
	CHECK(renderLevel(grid, nullptr).find("containers") == std::string::npos);
	paintLook(dressing(grid, 2, ROW), static_cast<int>(Container::Chest));
	paintLook(dressing(grid, 3, ROW), COFFIN);
	paintLook(dressing(grid, 4, ROW), PLAIN_COFFIN_BRUSH);
	paintLook(dressing(grid, 5, ROW), INSCRIBED_COFFIN_BRUSH);
	const std::string map = renderLevel(grid, nullptr);
	const size_t at = map.find("containers\n");
	REQUIRE(at != std::string::npos);
	CHECK(map.find(" 1 #.cmpi" + std::string(LAST + 2 - 6, '.') + "#", at) != std::string::npos); // cols 6 to 20
}

TEST_CASE("the editor's look brushes paint and pick the same look") {
	for (int brush = PLACED_UNSET; brush < LOOK_BRUSH_COUNT; brush++) {
		if (brush == PLACED_BARE)
			continue; // no bare look: a treasure always has a container
		Dressing d;
		paintLook(d, brush);
		CHECK(pickLookBrush(d) == brush);
		CHECK((d.lid == CoffinLid::Rolled || d.container == COFFIN));
	}
	Dressing d;
	paintLook(d, INSCRIBED_COFFIN_BRUSH);
	paintLook(d, static_cast<int>(Container::Basket)); // a basket clears the coffin's lid
	CHECK(d.lid == CoffinLid::Rolled);
	paintLook(d, PLACED_UNSET);
	CHECK_FALSE(hasPlacedContainer(d));
}

TEST_CASE("the campaign's placed coffins: on treasures in the tombs, some of them cursed") {
	int coffins = 0, cursed = 0;
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		const std::string file = campaignLevelFile(n);
		LevelGrid grid;
		REQUIRE(loadLevelFile(file.c_str(), grid).empty());
		auto layout = std::make_unique<DecorLayout>();
		scatterDecor(grid.cells, file.c_str(), n, grid.meta.zone, *layout);
		for (int i = 0; i < LEVEL_WIDTH * LEVEL_HEIGHT; i++) {
			const Dressing& d = grid.cells[i].dressing;
			if (!hasPlacedContainer(d))
				continue;
			CHECK_MESSAGE(isTreasure(grid.cells[i]), "lvl", n);
			CHECK(layout->treasure[i].kind == static_cast<Container>(d.container));
			if (d.lid != CoffinLid::Rolled)
				CHECK(layout->treasure[i].inscribed == (d.lid == CoffinLid::Inscribed));
			coffins += d.container == COFFIN ? 1 : 0;
			cursed += d.lid == CoffinLid::Inscribed ? 1 : 0;
		}
	}
	CHECK(coffins >= 4);
	CHECK(cursed >= 2);
}
