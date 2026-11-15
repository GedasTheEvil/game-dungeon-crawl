// Soft landing (plan soft-landing): a landing heap at the foot of a long drop, the soft thud and the puff of its
// matter on a fall of 2+ rows, a plain thud on a bare floor; the checker wants a heap under a fall of 3+ rows, the
// generator puts one there.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/level/campaign.h"
#include "../../src/level/decor.h"
#include "../../src/level/level_check.h"
#include "../../src/level/level_gen.h"
#include "../../src/level/zone_look.h"
#include <algorithm>
#include <string>

namespace {
int played(const SimWorld& world, WorldSound sound) {
	return static_cast<int>(std::count_if(world.said.begin(), world.said.end(), [sound](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Sound && e.sound == sound;
	}));
}

// Walks right until the player stands on a floor at row `row` (the fall over).
void fallTo(SimWorld& world, int row) {
	for (int t = 0; t < 400 && !(static_cast<int>(world.Y()) == row && !world.player.jump.falling); t++) {
		world.Step(1);
		world.Tick();
	}
	REQUIRE(static_cast<int>(world.Y()) == row);
	world.WaitTicks(2);
}
} // namespace

// tests/levels/soft_landing: a 4-row shaft at col 3 onto a sand heap (row 3), then a 2-row drop at col 6 onto bare
// floor (row 1).
TEST_CASE("soft landing: a soft thud and a puff on the heap, a plain thud on the bare floor") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/soft_landing"));
	REQUIRE(world.dungeon.PropAt(3, 3) == DECOR_HEAP_SAND);
	fallTo(world, 3);
	CHECK(played(world, WorldSound::LandSoft) == 1);
	CHECK(played(world, WorldSound::LandHard) == 0);
	CHECK(world.dungeon.LandingPuffs() == 1);
	CHECK(world.dungeon.LastLandingHeap() == DECOR_HEAP_SAND);
	world.Wait(LANDING_PUFF_MS + 100);
	fallTo(world, 1);
	CHECK(played(world, WorldSound::LandHard) == 1);
	CHECK(played(world, WorldSound::LandSoft) == 1);
	CHECK(world.dungeon.LastLandingHeap() == DECOR_HEAP_SAND); // no new puff on the bare floor
}

TEST_CASE("soft landing: a walk or a short step makes no thud") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/soft_landing"));
	world.Walk(1.f);
	world.WaitTicks(10);
	CHECK(played(world, WorldSound::LandSoft) + played(world, WorldSound::LandHard) == 0);
}

TEST_CASE("soft landing: the checker warns on a fall of 3+ rows onto a bare floor, not onto a heap") {
	LevelGrid grid;
	REQUIRE(loadLevelFile("tests/levels/soft_landing", grid).empty());
	CHECK(checkLevel(grid).bareLandings.empty());
	grid.cells[3 * LEVEL_WIDTH + 3].dressing.prop = PLACED_UNSET;
	const LevelReport r = checkLevel(grid);
	REQUIRE(r.bareLandings.size() == 1);
	CHECK(r.bareLandings[0] == 3 * LEVEL_WIDTH + 3);
	CHECK(std::any_of(r.warnings.begin(), r.warnings.end(), [](const std::string& w) {
		return w.find("a fall of 4 rows onto a bare floor at (col 3, row 3)") != std::string::npos;
	}));
}

TEST_CASE("soft landing: every zone has a heap; the generator leaves no bare landing") {
	for (int zone = 0; zone < ZONE_ID_COUNT; zone++)
		CHECK(isLandingHeap(heapOfZone(zone)));
	CHECK(heapOfZone(static_cast<int>(ZoneId::LakeOfFire)) == DECOR_HEAP_ASH);
	for (int zone = 0; zone < ZONE_ID_COUNT; zone += 3) {
		GenOptions options;
		options.seed = 11U + static_cast<unsigned>(zone);
		options.difficulty = 4;
		options.zone = zone;
		const GenResult result = generateLevel(options);
		REQUIRE(result.ok);
		CHECK(checkLevel(result.grid).bareLandings.empty());
		for (const Tile& t : result.grid.cells)
			if (isLandingHeap(t.dressing.prop))
				CHECK(t.dressing.prop == heapOfZone(zone));
	}
}
