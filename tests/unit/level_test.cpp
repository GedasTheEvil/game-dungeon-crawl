#include "../../external/doctest/doctest.h"
#include "../../src/world/level.h"
#include <cstdio>
#include <sstream>
#include <string>

namespace {
void checkSameCells(const LevelGrid& a, const LevelGrid& b) {
	for (int i = 0; i < LEVEL_WIDTH * LEVEL_HEIGHT; i++) {
		CHECK(a.cells[i].type == b.cells[i].type);
		CHECK(a.cells[i].attr == b.cells[i].attr);
		CHECK(a.cells[i].value == b.cells[i].value);
		CHECK(a.cells[i].structure == b.cells[i].structure);
	}
}

Tile water(Tile t, Structure s) {
	t.structure = s;
	return t;
}
} // namespace

TEST_CASE("a level survives a save and a load") {
	LevelGrid grid;
	grid.set(1, 0, Tile{});
	grid.set(3, 2, {Gate, 2, 1});
	grid.set(4, 2, water(Tile{Ladder, 0, 0}, Structure::HalfWater));
	grid.set(5, 2, water(Tile{}, Structure::DeepWater));
	grid.set(6, 2, slainObject(MonsterAnubisBoss)); // no type, but an attribute: still written
	grid.set(39, 46, {MonsterSpawn, MonsterBossScarab, 0});
	const std::string path = "build/unit_level_roundtrip";
	REQUIRE(saveLevelFile(path.c_str(), grid));

	LevelGrid loaded;
	int version = 0;
	CHECK(loadLevelFile(path.c_str(), loaded, &version).empty());
	CHECK(version == LEVEL_VERSION);
	checkSameCells(loaded, grid);
	std::remove(path.c_str());
}

TEST_CASE("a new grid is all wall") {
	LevelGrid grid;
	for (const Tile& t : grid.cells) {
		CHECK(isWall(t));
		CHECK_FALSE(hasObject(t));
	}
}

TEST_CASE("a v1 level converts: wall, open, objects in open cells, Area3D dropped") {
	std::ostringstream v1;
	v1 << LEVEL_CELL_COUNT << '\n';
	for (int i = 0; i < LEVEL_CELL_COUNT; i++) {
		const int col = i % LEVEL_WIDTH;
		const int row = i / LEVEL_WIDTH;
		if (row == 1 && col == 1)
			v1 << "1 0 0\n";
		else if (row == 1 && col == 2)
			v1 << "11 3 1\n"; // an open green gate
		else if (row == 1 && col == 3)
			v1 << "7 0 0\n"; // Area3D
		else if (row == 1 && col == 4)
			v1 << "1 14 0\n"; // a slain boss's spawn, as in save games
		else
			v1 << "0 0 0\n";
	}
	std::istringstream in(v1.str());
	LevelGrid grid;
	int version = 0;
	REQUIRE(readLevel(in, grid.cells, &version).empty());
	CHECK(version == 1);
	CHECK(isWall(grid.at(0, 1)));
	CHECK(isEmptyCell(grid.at(1, 1)));
	CHECK(grid.at(2, 1).type == Gate);
	CHECK(grid.at(2, 1).attr == 3);
	CHECK(gateState(grid.at(2, 1)) == GateState::Open);
	CHECK(grid.at(2, 1).structure == Structure::Empty);
	CHECK(isEmptyCell(grid.at(3, 1)));
	CHECK(slainMonster(grid.at(4, 1)) == MonsterAnubisBoss);

	// Written as v2 and read back: the same cells.
	std::stringstream v2;
	writeLevel(v2, grid.cells);
	LevelGrid again;
	REQUIRE(readLevel(v2, again.cells, &version).empty());
	CHECK(version == LEVEL_VERSION);
	checkSameCells(again, grid);
}

TEST_CASE("a broken v2 level is an error") {
	LevelGrid grid;
	std::istringstream badVersion("DCLEVEL 3 40 47");
	CHECK_FALSE(readLevel(badVersion, grid.cells).empty());
	std::istringstream badSize("DCLEVEL 2 30 47");
	CHECK_FALSE(readLevel(badSize, grid.cells).empty());
	std::istringstream shortRows("DCLEVEL 2 40 47 structure ####");
	CHECK_FALSE(readLevel(shortRows, grid.cells).empty());
}

TEST_CASE("setObject keeps the structure") {
	Tile cell = water(Tile{Key, 1, 0}, Structure::HalfWater);
	clearObject(cell);
	CHECK(cell.structure == Structure::HalfWater);
	CHECK_FALSE(hasObject(cell));
	setObject(cell, Tile{Treasure, 3, 1});
	CHECK(cell.structure == Structure::HalfWater);
	CHECK(cell.type == Treasure);
}

TEST_CASE("a missing level file is an error") {
	LevelGrid grid;
	CHECK_FALSE(loadLevelFile("build/no_such_level", grid).empty());
}

TEST_CASE("walls, deep water and closed gates are solid") {
	CHECK(isSolidTile(wallTile()));
	CHECK(isSolidTile(water(Tile{}, Structure::DeepWater)));
	CHECK_FALSE(isSolidTile(water(Tile{}, Structure::HalfWater)));
	CHECK(isSolidTile({Gate, 1, 0}));
	CHECK(isSolidTile({Gate, 1, 2})); // opening
	CHECK_FALSE(isSolidTile({Gate, 1, 1}));
	CHECK_FALSE(isSolidTile(Tile{}));
	CHECK_FALSE(isSolidTile({Ladder, 0, 0}));
}

TEST_CASE("teleporters pair by id") {
	LevelGrid grid;
	grid.set(2, 1, {Door, GateTeleport, 7});
	grid.set(30, 20, {Door, GateTeleport, 7});
	grid.set(10, 5, {Door, GateTeleport, 8});
	const int a = 1 * LEVEL_WIDTH + 2;
	const int b = 20 * LEVEL_WIDTH + 30;
	const int lone = 5 * LEVEL_WIDTH + 10;
	CHECK(teleportPartner(grid.cells, a) == b);
	CHECK(teleportPartner(grid.cells, b) == a);
	CHECK(teleportPartner(grid.cells, lone) == -1);
}

TEST_CASE("out of bounds reads as wall") {
	LevelGrid grid;
	CHECK(isWall(grid.at(-1, 0)));
	CHECK(isWall(grid.at(LEVEL_WIDTH, 0)));
	CHECK(isWall(grid.at(0, LEVEL_HEIGHT)));
}

TEST_CASE("tile states keep their numbers on disk") {
	CHECK(gateState({Gate, 1, 0}) == GateState::Closed);
	CHECK(gateState({Gate, 1, 1}) == GateState::Open);
	CHECK(gateState({Gate, 1, 2}) == GateState::Opening);
	CHECK(rockState({RockFall, 0, 1}) == RockState::Fallen);
	CHECK(rockState({RockFall, 0, 2}) == RockState::Falling);
	Tile gate{Gate, 1, 0};
	setGateState(gate, GateState::Opening);
	CHECK(gate.value == 2);
	Tile lever{Lever, 1, 0};
	CHECK_FALSE(leverPulled(lever));
	pullLever(lever);
	CHECK(lever.value == 1);
	CHECK(teleportPair({Door, GateTeleport, 7}) == 7);
}

TEST_CASE("the teleporter index agrees with the scan") {
	LevelGrid grid;
	grid.set(2, 1, {Door, GateTeleport, 7});
	grid.set(30, 20, {Door, GateTeleport, 7});
	grid.set(10, 5, {Door, GateTeleport, 8});
	grid.set(11, 5, {Door, GateTeleport, 7}); // a third one of pair 7: the checker warns, the scan picks the first
	TeleportPairs pairs(grid.cells);
	for (int i = 0; i < LEVEL_WIDTH * LEVEL_HEIGHT; i++)
		CHECK(pairs.Partner(i) == (isTeleporter(grid.cells[i]) ? teleportPartner(grid.cells, i) : -1));
}
