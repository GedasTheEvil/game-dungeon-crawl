// Sub-cells (plan level-subcells): the shape queries, the v6 shape layer, the editor's shape brushes and the test
// level tests/levels/subcells.

#include "sim_world.h"
#include "../../external/doctest/doctest.h"
#include "../../src/level/level_check.h"
#include "../../tools/editor/shape_brush.h"
#include <sstream>
#include <string>

namespace {
Tile shaped(Structure s, std::uint16_t shape) {
	Tile t;
	t.structure = s;
	t.shape = shape;
	return t;
}

std::string written(const LevelGrid& grid) {
	std::ostringstream out;
	writeLevel(out, grid.cells, grid.meta);
	return out.str();
}

std::string readError(const std::string& file) {
	LevelGrid grid;
	std::istringstream in(file);
	return readLevel(in, grid.cells);
}

void replace(std::string& s, const std::string& from, const std::string& to) {
	const size_t at = s.find(from);
	REQUIRE(at != std::string::npos);
	s.replace(at, from.size(), to);
}
} // namespace

TEST_CASE("solid at, floor and ceiling heights come from the shape's sub-cells") {
	const Tile step = shaped(Structure::Empty, 0x000f); // a quarter step over the floor
	CHECK(solidAt(step, 0.1f, 0.1f));
	CHECK_FALSE(solidAt(step, 0.1f, 0.3f));
	CHECK(floorHeight(step, 0.6f) == doctest::Approx(0.25f));
	CHECK(ceilingHeight(step, 0.6f) == doctest::Approx(1.f));

	const Tile stair = shaped(Structure::Empty, 0x08ce); // up to the right: 0, 1, 2, 3 quarters
	CHECK(floorHeight(stair, 0.1f) == doctest::Approx(0.f));
	CHECK(floorHeight(stair, 0.3f) == doctest::Approx(0.25f));
	CHECK(floorHeight(stair, 0.6f) == doctest::Approx(0.5f));
	CHECK(floorHeight(stair, 0.9f) == doctest::Approx(0.75f));
	CHECK(floorHeight(stair, 1.f) == doctest::Approx(0.75f)); // the cell's right edge counts as its last column

	const Tile crawl = shaped(Structure::Empty, 0xfff0); // a crawl space a quarter high
	CHECK(floorHeight(crawl, 0.5f) == doctest::Approx(0.f));
	CHECK(ceilingHeight(crawl, 0.5f) == doctest::Approx(0.25f));
	CHECK(solidAt(crawl, 0.5f, 0.9f));

	const Tile ledge = shaped(Structure::Empty, 0xf00f); // a step and a low ceiling: floor 0.25, ceiling 0.75
	CHECK(floorHeight(ledge, 0.5f) == doctest::Approx(0.25f));
	CHECK(ceilingHeight(ledge, 0.5f) == doctest::Approx(0.75f));

	const Tile lip = shaped(Structure::Empty, 0x0001); // only the leftmost column
	CHECK(floorHeight(lip, 0.f) == doctest::Approx(0.25f));
	CHECK(floorHeight(lip, 0.3f) == doctest::Approx(0.f));

	CHECK(floorHeight(Tile{}, 0.5f) == doctest::Approx(0.f));
	CHECK(ceilingHeight(Tile{}, 0.5f) == doctest::Approx(1.f));
	CHECK(floorHeight(wallTile(), 0.5f) == doctest::Approx(1.f));
	CHECK(ceilingHeight(wallTile(), 0.5f) == doctest::Approx(1.f));
	CHECK(solidAt(shaped(Structure::DeepWater, NO_SHAPE), 0.5f, 0.5f));
	CHECK_FALSE(hasShape(shaped(Structure::Wall, 0x000f))); // a wall takes no shape
}

TEST_CASE("a v6 level keeps its shapes, with their glyphs in the structure drawing") {
	LevelGrid grid;
	grid.set(2, 1, shaped(Structure::Empty, 0x000f));
	grid.set(3, 1, shaped(Structure::HalfWater, 0xff00));
	grid.set(4, 1, shaped(Structure::Sand, 0x08ce));
	grid.set(5, 1, shaped(Structure::Quicksand, 0xffff));
	grid.set(6, 1, Tile{});
	const std::string file = written(grid);
	CHECK(file.find("\nshapes 4\n2 1 000f\n3 1 ff00\n4 1 08ce\n5 1 ffff\n") != std::string::npos);
	CHECK(file.find("\n##_-;&.") != std::string::npos);

	LevelGrid loaded;
	std::istringstream in(file);
	int version = 0;
	REQUIRE(readLevel(in, loaded.cells, &version).empty());
	CHECK(version == LEVEL_VERSION);
	for (int i = 0; i < LEVEL_WIDTH * LEVEL_HEIGHT; i++) {
		CHECK(loaded.cells[i].structure == grid.cells[i].structure);
		CHECK(loaded.cells[i].shape == grid.cells[i].shape);
	}
}

TEST_CASE("a v5 level loads with no shapes") {
	LevelGrid grid;
	grid.set(2, 1, Tile{});
	grid.set(3, 1, shaped(Structure::HalfWater, NO_SHAPE));
	std::string v5 = written(grid);
	replace(v5, "DCLEVEL " + std::to_string(LEVEL_VERSION), "DCLEVEL 5");
	replace(v5, "shapes 0\n", "");
	LevelGrid loaded;
	std::istringstream in(v5);
	int version = 0;
	REQUIRE(readLevel(in, loaded.cells, &version).empty());
	CHECK(version == 5);
	for (const Tile& t : loaded.cells)
		CHECK_FALSE(hasShape(t));
	CHECK(loaded.at(3, 1).structure == Structure::HalfWater);

	// No shaped glyphs before v6.
	grid.set(2, 1, shaped(Structure::Empty, 0x000f));
	std::string shapedV5 = written(grid);
	replace(shapedV5, "DCLEVEL " + std::to_string(LEVEL_VERSION), "DCLEVEL 5");
	CHECK(readError(shapedV5).find("unknown structure '_'") != std::string::npos);
}

TEST_CASE("the shape layer and the structure drawing must agree") {
	LevelGrid grid;
	grid.set(2, 1, shaped(Structure::Empty, 0x000f));
	grid.set(3, 1, Tile{});
	const std::string good = written(grid);
	REQUIRE(readError(good).empty());

	std::string noShape = good;
	replace(noShape, "shapes 1\n2 1 000f\n", "shapes 0\n");
	CHECK(readError(noShape).find("shaped cell at 2 1 with no shape") != std::string::npos);

	std::string plain = good;
	replace(plain, "shapes 1\n2 1 000f\n", "shapes 2\n2 1 000f\n3 1 0003\n");
	CHECK(readError(plain).find("shape at 3 1 on a cell") != std::string::npos);

	std::string empty = good;
	replace(empty, "2 1 000f", "2 1 0000");
	CHECK(readError(empty).find("empty shape at 2 1") != std::string::npos);

	std::string notHex = good;
	replace(notHex, "2 1 000f", "2 1 00g1");
	CHECK(readError(notHex).find("not 4 hex digits") != std::string::npos);

	std::string missing = good;
	replace(missing, "shapes 1\n2 1 000f\n", "");
	CHECK(readError(missing).find("no shape layer") != std::string::npos);
}

TEST_CASE("the editor's shape brushes: a whole shape, single sub-cells, rock carved open") {
	Tile cell;
	paintShape(cell, SHAPE_BRUSHES[shapeBrushOf(0x00ff)], 0, 0);
	CHECK(cell.shape == 0x00ff);
	CHECK(cell.structure == Structure::Empty);

	const ShapeBrush solid{"", "", 0, ShapeTool::SolidSubcell};
	const ShapeBrush open{"", "", 0, ShapeTool::OpenSubcell};
	paintShape(cell, solid, 3, 3);
	CHECK(cell.shape == 0x80ff);
	paintShape(cell, open, 0, 0);
	CHECK(cell.shape == 0x80fe);

	Tile water = shaped(Structure::HalfWater, NO_SHAPE);
	paintShape(water, SHAPE_BRUSHES[shapeBrushOf(0x000f)], 0, 0);
	CHECK(water.structure == Structure::HalfWater); // the structure stays
	CHECK(water.shape == 0x000f);
	paintShape(water, SHAPE_BRUSHES[shapeBrushOf(NO_SHAPE)], 0, 0);
	CHECK_FALSE(hasShape(water));

	Tile rock = wallTile();
	paintShape(rock, solid, 1, 1); // all solid already
	CHECK(isWall(rock));
	paintShape(rock, SHAPE_BRUSHES[shapeBrushOf(NO_SHAPE)], 0, 0);
	CHECK(isWall(rock));
	paintShape(rock, open, 1, 1); // one sub-cell open: carved open round it
	CHECK(rock.structure == Structure::Empty);
	CHECK(rock.shape == (0xffff & ~subcellBit(1, 1)));

	Tile wall = wallTile();
	paintShape(wall, SHAPE_BRUSHES[shapeBrushOf(0xf000)], 0, 0);
	CHECK(wall.structure == Structure::Empty);
	CHECK(wall.shape == 0xf000);

	for (int i = 0; i < SHAPE_BRUSH_COUNT; i++) // every whole brush is found by its shape
		if (SHAPE_BRUSHES[i].tool == ShapeTool::Whole)
			CHECK(shapeBrushOf(SHAPE_BRUSHES[i].mask) == i);
}

TEST_CASE("the test level tests/levels/subcells: its shapes load, the checker passes, a save keeps them") {
	LevelGrid grid;
	int version = 0;
	REQUIRE(loadLevelFile("tests/levels/subcells", grid, &version).empty());
	CHECK(version == LEVEL_VERSION);
	CHECK(grid.at(3, 2).shape == 0x000f); // a quarter step
	CHECK(grid.at(8, 2).shape == 0x08ce); // the stair
	CHECK(grid.at(25, 2).shape == 0xfff0); // the crawl space
	CHECK(grid.at(10, 2).shape == 0x0008); // a lip
	CHECK(grid.at(19, 2).structure == Structure::HalfWater);
	CHECK(grid.at(19, 2).shape == 0x000f);
	CHECK(grid.at(22, 2).structure == Structure::Sand);
	CHECK_FALSE(hasShape(grid.at(2, 2)));
	const LevelReport report = checkLevel(grid);
	CHECK(report.valid);
	CHECK(report.warnings.empty());
	CHECK(renderLevel(grid, nullptr).find("#..____.____.____.~-~,;..__.#") != std::string::npos);

	SimWorld world; // the game carries the shapes, through a save too
	REQUIRE(world.Load("tests/levels/subcells"));
	CHECK(world.dungeon.Cell(8, 2).shape == 0x08ce);
	REQUIRE(world.dungeon.PlacePlayer(23, 2));
	world.SaveAndLoad();
	CHECK(world.dungeon.Cell(8, 2).shape == 0x08ce);
	CHECK(world.dungeon.Cell(19, 2).shape == 0x000f);
}
