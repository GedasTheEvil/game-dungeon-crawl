#include "../../external/doctest/doctest.h"
#include "../../src/world/level.h"
#include <cstdio>
#include <string>

TEST_CASE("a level survives a save and a load") {
	LevelGrid grid;
	grid.set(0, 0, {Wall, 0, 0});
	grid.set(3, 2, {Gate, 2, 1});
	grid.set(39, 46, {MonsterSpawn, MonsterBossScarab, 0});
	const std::string path = "build/unit_level_roundtrip";
	REQUIRE(saveLevelFile(path.c_str(), grid));

	LevelGrid loaded;
	CHECK(loadLevelFile(path.c_str(), loaded).empty());
	for (int i = 0; i < LEVEL_CELL_COUNT; i++) {
		CHECK(loaded.cells[i].type == grid.cells[i].type);
		CHECK(loaded.cells[i].attr == grid.cells[i].attr);
		CHECK(loaded.cells[i].value == grid.cells[i].value);
	}
	std::remove(path.c_str());
}

TEST_CASE("a missing level file is an error") {
	LevelGrid grid;
	CHECK_FALSE(loadLevelFile("build/no_such_level", grid).empty());
}

TEST_CASE("walls and closed gates are solid") {
	CHECK(isSolidTile({Wall, 0, 0}));
	CHECK(isSolidTile({Gate, 1, 0}));
	CHECK(isSolidTile({Gate, 1, 2})); // opening
	CHECK_FALSE(isSolidTile({Gate, 1, 1}));
	CHECK_FALSE(isSolidTile({Empty, 0, 0}));
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
	CHECK(grid.at(-1, 0).type == Wall);
	CHECK(grid.at(LEVEL_WIDTH, 0).type == Wall);
	CHECK(grid.at(0, LEVEL_HEIGHT).type == Wall);
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
