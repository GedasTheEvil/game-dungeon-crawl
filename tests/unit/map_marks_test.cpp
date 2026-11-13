// The draft map's marks (plan map-legend-sprites, src/level/map_marks.h): what a cell shows (its object, else its
// fixture, else a taken treasure's container, else its prop), the sprite of each look, and the guards against the
// next gap: every tile type and every prop the player can use has a mark, every mark a name and a sprite.
#include "../../external/doctest/doctest.h"
#include "../../src/level/map_marks.h"
#include "../../src/level/tile_defs.h"
#include "sim_world.h"
#include <set>
#include <string>

namespace {
constexpr int ROW = 1; // tests/levels/map_marks: the walk
constexpr int FREE_COL = 23; // ... a cell with no object and no fixture

MapMark markAt(const SimWorld& world, int col, int row = ROW) {
	const Dungeon& d = world.dungeon;
	return cellMark(d.Cell(col, row), d.PropAt(col, row), d.ContainerEmptied(col, row)).mark;
}
} // namespace

TEST_CASE("map marks: a shrine, the Stargate, its dial and a brazier show on the map") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/map_marks"));
	std::set<MapMark> shrines;
	for (const GodDef& god : GODS) {
		REQUIRE(world.dungeon.PlaceDecor(FREE_COL, ROW, god.decor));
		const MapMark mark = markAt(world, FREE_COL);
		CHECK(mark != MapMark::None);
		CHECK(std::string(markDef(mark).name) == std::string("Shrine of ") + god.name);
		shrines.insert(mark);
	}
	CHECK(shrines.size() == GOD_COUNT); // a mark a god
	REQUIRE(world.dungeon.PlaceDecor(FREE_COL, ROW, DECOR_STARGATE));
	CHECK(markAt(world, FREE_COL) == MapMark::Stargate);
	REQUIRE(world.dungeon.PlaceDecor(FREE_COL, ROW, DECOR_DIAL));
	CHECK(markAt(world, FREE_COL) == MapMark::Dial);
	REQUIRE(world.dungeon.PlaceDecor(FREE_COL, ROW, DECOR_BRAZIER));
	CHECK(markAt(world, FREE_COL) == MapMark::Brazier);
	REQUIRE(world.dungeon.PlaceDecor(FREE_COL, ROW, DECOR_CAT)); // plain decor stays off the map
	CHECK(markAt(world, FREE_COL) == MapMark::None);
	// The ceiling traps of the level (fixtures).
	CHECK(markAt(world, 32) == MapMark::CeilingSpikes);
	CHECK(markAt(world, 34) == MapMark::CeilingDeath);
}

TEST_CASE("map marks: the object first, then the fixture, then a taken treasure, then the prop") {
	Tile cell = cellWith({Lever, 1, 0});
	CHECK(cellMark(cell, DECOR_SHRINE_ISIS, true).mark == MapMark::Lever);
	setObject(cell, Tile{Key, 2, 0});
	const ShownMark key = cellMark(cell, DECOR_SHRINE_ISIS, false);
	CHECK(key.mark == MapMark::Key);
	CHECK(key.thing.attr == 2);
	CHECK(cellMark(Tile{}, DECOR_SHRINE_ISIS, true).mark == MapMark::TreasureTaken);
	CHECK(cellMark(Tile{}, DECOR_SHRINE_ISIS, false).mark == MapMark::ShrineIsis);
	CHECK(cellMark(Tile{Treasure, 1, 0}, -1, false).mark == MapMark::Treasure);
	CHECK(cellMark(Tile{}, -1, false).mark == MapMark::None);
}

TEST_CASE("map marks: a sphinx gate and an answered one differ") {
	const MapMark open = thingMark(Tile{Door, GateRiddle, 0});
	const MapMark answered = thingMark(Tile{Door, GateEmpty, 0});
	CHECK(open == MapMark::SphinxGate);
	CHECK(answered == MapMark::SphinxGateAnswered);
	CHECK(markSprite({open, Tile{Door, GateRiddle, 0}}).cell != markSprite({answered, Tile{Door, GateEmpty, 0}}).cell);
	CHECK(std::string(markDef(open).name) != markDef(answered).name);
}

TEST_CASE("map marks: the second looks, the turns and the lock colours") {
	const auto sprite = [](const Tile& thing) { return markSprite({thingMark(thing), thing}); };
	// Two looks.
	Tile gate{Gate, 2, 0};
	const int closed = sprite(gate).cell;
	CHECK(closed == static_cast<int>(MapMark::Gate));
	setGateState(gate, GateState::Opening); // still shut
	CHECK(sprite(gate).cell == closed);
	setGateState(gate, GateState::Open);
	CHECK(sprite(gate).cell == markDef(MapMark::Gate).stateCell);
	CHECK(sprite(gate).lock == 2);
	Tile bossGate{Gate, BOSS_LOCK, 0};
	CHECK(sprite(bossGate).lock == 0); // its colours are in the sprite
	setGateState(bossGate, GateState::Open);
	CHECK(sprite(bossGate).cell == markDef(MapMark::BossGate).stateCell);
	CHECK(sprite(Tile{Lever, 1, 0}).cell == static_cast<int>(MapMark::Lever));
	CHECK(sprite(Tile{Lever, 1, 1}).cell == markDef(MapMark::Lever).stateCell);
	Tile rock{RockFall, 0, 0};
	setRockState(rock, RockState::Fallen);
	CHECK(sprite(rock).cell == markDef(MapMark::RockFall).stateCell);
	// Lock colours.
	for (int colour = 1; colour <= LOCK_COLOUR_COUNT; colour++) {
		CHECK(sprite(Tile{Key, colour, 0}).lock == colour);
		CHECK(sprite(Tile{SunDisc, colour, 0}).lock == colour);
	}
	CHECK(sprite(Tile{Ladder, 3, 0}).lock == 0);
	// Turns: a quarter turn clockwise a step of BeamDir; a gust blowing left is turned round.
	for (int way = 0; way < BEAM_DIRS; way++) {
		CHECK(sprite(Tile{LightShaft, way, 0}).turns == way);
		CHECK(sprite(Tile{Mirror, 0, way}).turns == way);
	}
	CHECK(sprite(Tile{SandGust, 0, 0}).turns == 0);
	CHECK(sprite(Tile{SandGust, GUST_LEFT, 0}).turns == 2);
}

TEST_CASE("map marks: every tile type but a monster has a mark") {
	for (int type = NoObject + 1; type < TILE_TYPE_COUNT; type++) {
		if (!isTileType(type) || type == MonsterSpawn)
			continue;
		CAPTURE(type);
		CHECK(thingMark(Tile{type, 1, 0}) != MapMark::None);
	}
	CHECK(thingMark(Tile{}) == MapMark::None);
	CHECK(thingMark(Tile{MonsterSpawn, MonsterMimic, 0}) == MapMark::Treasure); // the chest it pretends to be
	CHECK(thingMark(Tile{MonsterSpawn, MonsterRat, 0}) == MapMark::None);
}

TEST_CASE("map marks: every prop the player can use has a mark") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/map_marks"));
	world.Give(ItemKind::SelfBow); // the brazier's verb needs a bow
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(static_cast<float>(FREE_COL) + 0.5f));
	int used = 0;
	for (int decor = 0; decor < DECOR_COUNT; decor++) {
		REQUIRE(world.dungeon.PlaceDecor(FREE_COL, ROW, decor));
		if (world.dungeon.InteractVerb() == Interaction::None)
			continue;
		CAPTURE(DECOR_NAMES[decor]);
		CHECK(propMark(decor) != MapMark::None);
		used++;
	}
	CHECK(used >= GOD_COUNT + 3); // the shrines, the Stargate, its dial, the brazier
}

TEST_CASE("map marks: every mark has a name and a sprite of its own inside the sheet") {
	CHECK(MAP_MARK_CELLS >= MARK_COUNT);
	std::set<int> cells;
	std::set<std::string> names;
	for (int k = 0; k < MARK_COUNT; k++) {
		const MarkDef& def = MARK_DEFS[k];
		CAPTURE(k);
		REQUIRE(def.name != nullptr);
		CHECK(def.name[0] != '\0');
		names.insert(def.name);
		cells.insert(k);
		if (def.stateCell != NO_STATE_CELL) {
			CHECK(def.stateCell >= MARK_COUNT); // after the marks' own cells
			CHECK(def.stateCell < MAP_MARK_CELLS);
			CHECK(cells.insert(def.stateCell).second);
		}
	}
	CHECK(names.size() == MARK_COUNT);
}
