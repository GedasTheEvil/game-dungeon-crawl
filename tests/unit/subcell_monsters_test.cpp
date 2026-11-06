// Monsters against sub-cells (plan subcell-monsters): steps and crawl spaces by the kind's height and step, on
// tests/levels/subcell_monsters (its .txt draws it); the scenario: tests/scenarios/solved/subcell_monsters.txt.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/sim/model_info.h"
#include "../../src/level/climb_path.h"
#include <algorithm>
#include <cmath>

namespace {
// The halls of tests/levels/subcell_monsters, the shaped cell at SHAPED in each.
constexpr int SHAPED = 11;
constexpr int GIANT_BAT_AT_CRAWL = 12, BAT_AT_CRAWL = 10, RAT_AT_STEP = 8, GIANT_RAT_AT_STEP = 6, MUMMY_AT_CRAWL = 4,
			  RAT_AT_CRAWL = 2;
constexpr const char* LEVEL = "tests/levels/subcell_monsters";

const Monster& spawned(SimWorld& world, int col, int row) {
	(void)world.dungeon.SpawnMonster(col, row); // false: already in, spawned in view
	for (int i = 0; i < MAX_MONSTERS; i++) {
		const Monster& mon = world.dungeon.Monsters()[i];
		if (mon.Active() && mon.Col() == col && mon.SpawnRow() == row)
			return mon;
	}
	FAIL("no monster at ", col, " ", row);
	return world.dungeon.Monsters()[0];
}

void load(SimWorld& world) {
	world.player.god = true;
	REQUIRE(world.Load(LEVEL));
}

Tile shaped(std::uint16_t shape) {
	Tile t;
	t.shape = shape;
	return t;
}

BodyFit bodyOf(int type) { return {monsterKind(type)->heightQuarters, monsterKind(type)->stepQuarters}; }
} // namespace

TEST_CASE("every kind is at least as tall as its model; only a narrow one fits through a crawl space") {
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) {
		const MonsterKind& kind = *monsterKind(id);
		CAPTURE(std::string(kind.label));
		ModelInfo info;
		REQUIRE(LoadClips(kind.model, ClipFilesOf(kind), info));
		const float tiles = info.referenceTop * kind.scale / RenderConfig::TILE_SIZE;
		CHECK(static_cast<float>(kind.heightQuarters) * SUBCELL_SIZE >= tiles);
		if (kind.heightQuarters == 1)
			CHECK(info.HalfWidth() * kind.scale / RenderConfig::TILE_SIZE <= SUBCELL_SIZE);
	}
	CHECK(monsterKind(MonsterRat)->heightQuarters == 1);
	CHECK(monsterKind(MonsterScarab)->heightQuarters == 1);
	for (const int large : {MonsterMummy, MonsterHippo, MonsterCrocodile, MonsterAnubis})
		CHECK(monsterKind(large)->heightQuarters > 1);
}

TEST_CASE("a leaper takes a quarter step, a walker none; a leaping boss half a tile, never less than its kin") {
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) {
		const MonsterKind& kind = *monsterKind(id);
		CAPTURE(std::string(kind.label));
		CHECK(kind.stepQuarters == (leaps(kind.locomotion) ? std::max(kind.stepQuarters, 1) : 0));
		if (kind.isBoss())
			CHECK(kind.stepQuarters >= monsterKind(kind.kin)->stepQuarters);
	}
	CHECK(monsterKind(MonsterBossScarab)->stepQuarters == 2);
	CHECK(monsterKind(MonsterBabi)->stepQuarters == 2);
	CHECK(monsterKind(MonsterBabi)->stepQuarters > monsterKind(MonsterGiantRat)->stepQuarters);

	const Tile quarter = shaped(0x000f), half = shaped(0x00ff), crawl = shaped(0xfff0);
	for (int k = 0; k < SUBCELLS; k++) {
		CHECK_FALSE(columnFits(quarter, k, 0.f, bodyOf(MonsterRat)));
		CHECK(columnFits(quarter, k, 0.f, bodyOf(MonsterGiantRat)));
		CHECK_FALSE(columnFits(half, k, 0.f, bodyOf(MonsterGiantRat)));
		CHECK(columnFits(half, k, 0.f, bodyOf(MonsterBabi))); // the boss jumps higher than the giant rat
		CHECK(columnFits(crawl, k, 0.f, bodyOf(MonsterRat)));
		CHECK_FALSE(columnFits(crawl, k, 0.f, bodyOf(MonsterMummy)));
	}
}

TEST_CASE("the climb path goes through a crawl space for a rat, not for a mummy") {
	LevelGrid g{};
	REQUIRE(loadLevelFile(LEVEL, g).empty());
	ClimbMap rat, mummy;
	rat.Build(g.cells, SHAPED + 3, RAT_AT_CRAWL, ClimbAbility{false, false, bodyOf(MonsterRat)}, CLIMB_PATH_MAX);
	mummy.Build(g.cells, SHAPED + 3, MUMMY_AT_CRAWL, ClimbAbility{false, false, bodyOf(MonsterMummy)}, CLIMB_PATH_MAX);
	CHECK(rat.Steps(3, RAT_AT_CRAWL) == SHAPED);
	CHECK(mummy.Steps(6, MUMMY_AT_CRAWL) == -1);
	ClimbMap walker, leaper;
	walker.Build(g.cells, SHAPED + 3, RAT_AT_STEP, ClimbAbility{false, false, bodyOf(MonsterRat)}, CLIMB_PATH_MAX);
	leaper.Build(
		g.cells,
		SHAPED + 3,
		GIANT_RAT_AT_STEP,
		ClimbAbility{false, true, bodyOf(MonsterGiantRat)},
		CLIMB_PATH_MAX
	);
	CHECK(walker.Steps(3, RAT_AT_STEP) == -1);
	CHECK(leaper.Steps(3, GIANT_RAT_AT_STEP) == SHAPED);
}

TEST_CASE("a rat runs through a crawl space to the player; a mummy stops at it") {
	SimWorld world;
	load(world);
	REQUIRE(world.dungeon.PlacePlayer(SHAPED + 4, RAT_AT_CRAWL));
	world.Tick();
	const Monster& rat = spawned(world, 8, RAT_AT_CRAWL);
	world.Wait(8000);
	CHECK(rat.CentreX() > static_cast<float>(SHAPED + 1));

	// The mummy woken next to it, then the player through to the far side.
	REQUIRE(world.dungeon.PlacePlayer(10, MUMMY_AT_CRAWL));
	world.Tick();
	const Monster& mummy = spawned(world, 9, MUMMY_AT_CRAWL);
	world.Wait(3000);
	REQUIRE(mummy.Alerted());
	REQUIRE(world.dungeon.PlacePlayer(SHAPED + 2, MUMMY_AT_CRAWL));
	world.Wait(8000);
	CHECK(mummy.Right() <= static_cast<float>(SHAPED) + 0.01f);
	CHECK(mummy.Right() > static_cast<float>(SHAPED) - 0.1f); // it came up to the opening
	CHECK(mummy.StepUp() == 0.f);
}

TEST_CASE("a giant rat jumps onto a quarter step and on; a rat does not get onto it") {
	SimWorld world;
	load(world);
	REQUIRE(world.dungeon.PlacePlayer(SHAPED + 5, GIANT_RAT_AT_STEP));
	world.Tick();
	const Monster& giant = spawned(world, 8, GIANT_RAT_AT_STEP);
	float highest = 0.f;
	for (int t = 0; t < 8000 / UPDATE_TICK_MS; t++) {
		world.Tick();
		highest = std::max(highest, giant.StepUp());
	}
	CHECK(highest == doctest::Approx(SUBCELL_SIZE));
	CHECK(giant.CentreX() > static_cast<float>(SHAPED + 1));

	REQUIRE(world.dungeon.PlacePlayer(SHAPED + 4, RAT_AT_STEP));
	world.Tick();
	const Monster& rat = spawned(world, 8, RAT_AT_STEP);
	world.Wait(8000);
	CHECK(rat.Right() <= static_cast<float>(SHAPED) + 0.01f); // a coward with no way to the player stays off
	CHECK(rat.StepUp() == 0.f);
}

TEST_CASE("a bat flies through a crawl space, low under its ceiling; a giant bat does not") {
	SimWorld world;
	load(world);
	for (const int row : {BAT_AT_CRAWL, GIANT_BAT_AT_CRAWL}) {
		CAPTURE(row);
		REQUIRE(world.dungeon.PlacePlayer(SHAPED + 1, row));
		world.Tick();
		CHECK_FALSE(world.WalkTo(static_cast<float>(SHAPED))); // the player's head does not fit: in sight of the roost
		const Monster& bat = spawned(world, SHAPED - 1, row);
		float farthest = 0.f, highestInside = 0.f;
		for (int t = 0; t < 6000 / UPDATE_TICK_MS; t++) {
			world.Tick();
			farthest = std::max(farthest, bat.Right());
			if (bat.CentreX() > static_cast<float>(SHAPED) && bat.CentreX() < static_cast<float>(SHAPED + 1))
				highestInside = std::max(highestInside, bat.TopY() - static_cast<float>(row));
		}
		if (row == BAT_AT_CRAWL) {
			CHECK(farthest > static_cast<float>(SHAPED + 1));
			CHECK(highestInside <= SUBCELL_SIZE + 0.02f);
		} else
			CHECK(farthest < static_cast<float>(SHAPED) + 0.05f);
	}
}
