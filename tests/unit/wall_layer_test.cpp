// The wall layer (plan wall-layer): the level format v3 and its v2 conversion, the checker's rules for a fixture and an
// object in one cell, interact over both, and a walker in front of a fixture.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/climb_path.h"
#include "../../src/world/level_check.h"
#include "../../src/world/spike_cycle.h"
#include "../../src/world/tile_defs.h"
#include "sim_world.h"
#include <sstream>
#include <string>

namespace {
// The current text of cells as v2: no wall list, the fixtures in the object list (as v2 files had them), no decor or
// decal lists (the cells have none).
std::string asV2(const Tile* cells) {
	std::ostringstream v3;
	writeLevel(v3, cells);
	std::istringstream in(v3.str());
	std::string line, head, structure, things;
	int count = 0;
	std::getline(in, head);
	std::getline(in, line); // the zone, v5 on
	const std::string version = " " + std::to_string(LEVEL_VERSION) + " ";
	head.replace(head.find(version), version.size(), " 2 ");
	while (std::getline(in, line)) {
		const bool list = line.rfind("wall ", 0) == 0 || line.rfind("objects ", 0) == 0 ||
						  line.rfind("decor ", 0) == 0 || line.rfind("decals ", 0) == 0;
		if (list)
			continue;
		const bool thing = line.find(' ') != std::string::npos;
		(thing ? things : structure) += line + '\n';
		count += thing ? 1 : 0;
	}
	return head + '\n' + structure + "objects " + std::to_string(count) + '\n' + things;
}

bool sameCell(const Tile& a, const Tile& b) {
	return a.type == b.type && a.attr == b.attr && a.value == b.value && a.structure == b.structure &&
		   a.fixture.type == b.fixture.type && a.fixture.attr == b.fixture.attr && a.fixture.value == b.fixture.value;
}

// A corridor on row 1, entrance at col 1, exit at col 11, wall spikes at col 5 with the object in front of them.
LevelGrid spikesOver(const Tile& object) {
	LevelGrid grid;
	for (int col = 1; col < 12; col++)
		grid.set(col, 1, Tile{});
	grid.set(1, 1, Tile{Door, GateEntrance, 0});
	grid.set(11, 1, Tile{Door, GateExit, 0});
	Tile cell = object;
	setFixture(cell, Fixture{WallSpikes, 0, 0});
	grid.set(5, 1, cell);
	return grid;
}

bool warnsInFront(const LevelReport& r) {
	for (const std::string& w : r.warnings)
		if (w.find("in front of wall spikes") != std::string::npos)
			return true;
	return false;
}
} // namespace

TEST_CASE("v2 to v3: every campaign level's wall types move to the wall layer, cell by cell, the rest stays") {
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		LevelGrid v3;
		int version = 0;
		REQUIRE(loadLevelFile(campaignLevelFile(n).c_str(), v3, &version).empty());
		CHECK_MESSAGE(version == LEVEL_VERSION, "lvl", n);
		std::istringstream in(asV2(v3.cells));
		LevelGrid v2;
		REQUIRE(readLevel(in, v2.cells, &version).empty());
		CHECK(version == 2);
		for (int i = 0; i < LEVEL_WIDTH * LEVEL_HEIGHT; i++) {
			CHECK_MESSAGE(sameCell(v2.cells[i], v3.cells[i]), "lvl", n, " cell ", i);
			CHECK_FALSE(isWallType(v3.cells[i].type)); // no wall type left in the object layer
			CHECK((!hasFixture(v3.cells[i]) || isWallType(v3.cells[i].fixture.type)));
		}
	}
}

TEST_CASE("a v3 level keeps both layers of a cell, and the wall spikes' live state") {
	LevelGrid grid;
	Tile chest{Treasure, 3, 0};
	setFixture(chest, Fixture{Inscription, 4, 0});
	grid.set(2, 1, chest);
	Tile spikes{MonsterSpawn, MonsterRat, 0};
	Fixture trap{WallSpikes, static_cast<int>(SpikeSpeed::Slow), 5};
	setSpikeState(trap, SpikeState::Telegraph);
	setFixture(spikes, trap);
	grid.set(3, 1, spikes);
	grid.set(4, 1, cellWith(Tile{Lever, 2, 1}));
	std::stringstream file;
	writeLevel(file, grid.cells);
	LevelGrid loaded;
	int version = 0;
	REQUIRE(readLevel(file, loaded.cells, &version).empty());
	CHECK(version == LEVEL_VERSION);
	for (int i = 0; i < LEVEL_WIDTH * LEVEL_HEIGHT; i++)
		CHECK(sameCell(loaded.cells[i], grid.cells[i]));
	CHECK(spikePhase(loaded.at(3, 1).fixture) == 5);
	CHECK(spikeState(loaded.at(3, 1).fixture) == SpikeState::Telegraph);
	CHECK(leverPulled(loaded.at(4, 1).fixture));
	CHECK(loaded.at(4, 1).type == NoObject);
}

TEST_CASE("a v3 wall list holds only wall types") {
	std::ostringstream v3;
	LevelGrid grid;
	grid.set(2, 1, Tile{});
	writeLevel(v3, grid.cells);
	std::string text = v3.str();
	text.replace(text.find("wall 0\n"), 7, "wall 1\n2 1 8 3 0\n"); // a chest
	std::istringstream in(text);
	LevelGrid loaded;
	CHECK(readLevel(in, loaded.cells).find("in the wall layer") != std::string::npos);
}

TEST_CASE("a save game keeps the wall spikes' phase and state") {
	SimWorld world; // tests/levels/wall_spikes: phase 0 at col 5, phase 4 at col 11
	REQUIRE(world.Load("tests/levels/wall_spikes"));
	world.WaitTicks(30);
	const Fixture before = world.dungeon.Cell(11, 2).fixture;
	REQUIRE(before.type == WallSpikes);
	world.SaveAndLoad();
	const Fixture after = world.dungeon.Cell(11, 2).fixture;
	CHECK(after.type == WallSpikes);
	CHECK(spikePhase(after) == 4);
	CHECK(spikeState(after) == spikeState(before));
}

TEST_CASE("interact over a chest under an inscription: the chest first, the inscription with the next press") {
	SimWorld world; // tests/levels/wall_layer: the chest under inscription 1 at col 3
	world.player.god = true;
	REQUIRE(world.inscriptions.Load(CONTENT_DIR "inscriptions").empty());
	REQUIRE(world.Load("tests/levels/wall_layer"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(3.5f));
	world.WaitTicks(5);
	world.OpenChest();
	world.WaitTicks(1);
	CHECK(world.Told("Found: "));
	CHECK(world.dungeon.Cell(3, 2).type == NoObject);
	CHECK(world.dungeon.Cell(3, 2).fixture.type == Inscription); // the chest goes, the inscription stays
	CHECK(world.journal.Inscriptions().empty());
	world.Interact();
	world.WaitTicks(1);
	REQUIRE(world.journal.Inscriptions().size() == 1);
	CHECK(world.journal.Inscriptions()[0].id == 1);
}

TEST_CASE("a lever behind a monster spawn: the monster comes, the player pulls the lever") {
	SimWorld world; // tests/levels/wall_layer: an egg cluster in front of the red lever at col 6, the red gate at col 8
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/wall_layer"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(6.5f));
	world.WaitTicks(5);
	REQUIRE(world.dungeon.NearestMonster() != nullptr);
	world.Interact();
	world.WaitTicks(1);
	CHECK(leverPulled(world.dungeon.Cell(6, 2).fixture));
	world.Wait(3000);
	CHECK(world.WalkTo(9.5f)); // through the open gate
}

TEST_CASE("a walker goes through an inscription's or a lever's cell, the checker's player too") {
	LevelGrid grid;
	for (int col = 1; col < 8; col++)
		grid.set(col, 1, Tile{});
	grid.set(3, 1, cellWith(Tile{Inscription, 1, 0}));
	grid.set(4, 1, cellWith(Tile{Lever, 1, 1}));
	CHECK_FALSE(walkerBlockedAt(grid.cells, 3, 1));
	CHECK_FALSE(walkerBlockedAt(grid.cells, 4, 1));
	CHECK_FALSE(isSolidTile(grid.at(3, 1)));
	grid.set(1, 1, Tile{Door, GateEntrance, 0});
	grid.set(7, 1, Tile{Door, GateExit, 0});
	CHECK(checkLevel(grid).valid);
}

TEST_CASE("the checker: no spawn, ladder, gate, door or chest in front of wall spikes; a key or a floor trap is fine") {
	CHECK_FALSE(warnsInFront(checkLevel(spikesOver(Tile{}))));
	CHECK_FALSE(warnsInFront(checkLevel(spikesOver(Tile{Key, 1, 0}))));
	CHECK_FALSE(warnsInFront(checkLevel(spikesOver(Tile{Spike, 0, 0}))));
	CHECK_FALSE(warnsInFront(checkLevel(spikesOver(Tile{DartPlate, 0, 0}))));
	CHECK(warnsInFront(checkLevel(spikesOver(Tile{MonsterSpawn, MonsterRat, 0}))));
	CHECK(warnsInFront(checkLevel(spikesOver(Tile{Ladder, 0, 0}))));
	CHECK(warnsInFront(checkLevel(spikesOver(Tile{Gate, 1, 0}))));
	CHECK(warnsInFront(checkLevel(spikesOver(Tile{Door, GateRiddle, 0}))));
	CHECK(warnsInFront(checkLevel(spikesOver(Tile{Treasure, 3, 0}))));
}

TEST_CASE("one glyph a cell: the object, else the fixture; the --map drawing adds the wall layer") {
	Tile both{Treasure, 3, 0};
	setFixture(both, Fixture{Inscription, 1, 0});
	CHECK(tileGlyph(both) == '$');
	CHECK(tileGlyph(cellWith(Tile{Inscription, 1, 0})) == 'i');
	LevelGrid grid;
	grid.set(1, 1, Tile{Door, GateEntrance, 0});
	grid.set(2, 1, both);
	grid.set(3, 1, Tile{Door, GateExit, 0});
	CHECK(renderLevel(grid, nullptr) == " 1 #S$E####################################\nwall\n"
										" 1 #.i.####################################\n");
}
