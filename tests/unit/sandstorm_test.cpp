// Sandstorm halls (plan sandstorm-halls): sand gusts on a cycle push the player downwind, faster than the walk; a
// shelter (a cell without a gust) keeps them; the haze closes in and the wind drowns the other sounds; the checker
// wants each run crossable against the wind in one calm and nothing bad downwind.
#include "../../external/doctest/doctest.h"
#include "../../src/world/level_check.h"
#include "../../src/world/spike_cycle.h"
#include "sim_world.h"
#include <algorithm>
#include <string>

namespace {
// tests/levels/sandstorm: row 2, cols 1-38; gusts blowing left at cols 8-10 (stone) and 21-22 (sand, 20-24), phase 0.
constexpr int GUST_COL = 9, ROW = 2;

SpikeState stateAt(SimWorld& world) { return spikeState(world.dungeon.Cell(GUST_COL, ROW).fixture); }

// Ticks until the gust comes into state s, at its start (at most a minute of game time).
void waitFor(SimWorld& world, SpikeState s) {
	int t = 0;
	for (; t < 60000 && stateAt(world) == s; t += UPDATE_TICK_MS)
		world.Tick();
	for (; t < 60000 && stateAt(world) != s; t += UPDATE_TICK_MS)
		world.Tick();
	REQUIRE(stateAt(world) == s);
}

bool heard(const SimWorld& world, WorldSound sound) {
	return std::any_of(world.said.begin(), world.said.end(), [sound](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Sound && e.sound == sound;
	});
}

bool noted(const SimWorld& world, FieldNote note) {
	return std::any_of(world.said.begin(), world.said.end(), [note](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Note && e.note == note;
	});
}

void load(SimWorld& world) {
	REQUIRE(world.Load("tests/levels/sandstorm"));
	world.WaitTicks(30);
}

// A corridor on row 2, cols 1-18, entrance at 1, exit at 18; gusts from col 5 on, one per entry of attrs, phase 0.
LevelGrid corridor(const std::vector<int>& attrs, Structure floor = Structure::Empty) {
	LevelGrid grid;
	for (int col = 1; col < 19; col++)
		grid.set(col, 2, Tile{});
	grid.set(1, 2, Tile{Door, GateEntrance, 0});
	grid.set(18, 2, Tile{Door, GateExit, 0});
	for (size_t k = 0; k < attrs.size(); k++) {
		Tile cell;
		cell.structure = floor;
		setFixture(cell, Fixture{SandGust, attrs[k], 0});
		grid.set(5 + static_cast<int>(k), 2, cell);
	}
	return grid;
}

bool warns(const LevelReport& r, const char* part) {
	return std::any_of(r.warnings.begin(), r.warnings.end(), [part](const std::string& w) {
		return w.find(part) != std::string::npos;
	});
}
} // namespace

TEST_CASE("sandstorm: the gust's cycle, its tell and its sounds") {
	const Fixture f{SandGust, 0, 0};
	CHECK(spikePeriodMs(f) == GUST_PERIOD_MS[0]);
	CHECK(spikeStateAt(f, 0) == SpikeState::Extended);
	CHECK(spikeStateAt(f, GUST_OUT_MS[0]) == SpikeState::Retracted);
	CHECK(spikeStateAt(f, GUST_PERIOD_MS[0] - GUST_TELEGRAPH_MS) == SpikeState::Telegraph);
	CHECK(gustDir(f) == 1);
	CHECK(gustDir(Fixture{SandGust, GUST_LEFT + 2, 0}) == -1);
	CHECK(gustSpeed(Fixture{SandGust, GUST_LEFT + 2, 0}) == SpikeSpeed::Slow);
	CHECK(gustStrength(f, GUST_OUT_MS[0]) == doctest::Approx(GUST_HAZE_CALM));
	CHECK(gustStrength(f, 0) == 1.f);

	SimWorld world;
	load(world);
	REQUIRE(world.WalkTo(6.f)); // in view of the hall: heard
	waitFor(world, SpikeState::Telegraph);
	waitFor(world, SpikeState::Extended);
	CHECK(heard(world, WorldSound::GustRise));
	CHECK(heard(world, WorldSound::GustBlow));
}

TEST_CASE("sandstorm: a gust pushes the player downwind against the walk; the calm lets them cross; a shelter holds") {
	SimWorld world;
	load(world);
	REQUIRE(world.WalkTo(7.5f)); // the shelter before the run
	waitFor(world, SpikeState::Extended);
	const float sheltered = world.X();
	world.Wait(GUST_OUT_MS[0] / 2);
	CHECK(world.X() == sheltered); // no wind behind the pillar
	CHECK_FALSE(noted(world, FieldNote::Sandstorm));

	waitFor(world, SpikeState::Retracted);
	REQUIRE(world.WalkTo(9.5f)); // into the run in the calm: the walk as anywhere
	waitFor(world, SpikeState::Extended);
	const float x = world.X();
	world.HoldWalk(1, 500); // walking against the wind loses ground
	CHECK(world.X() < x - 0.2f);
	CHECK(noted(world, FieldNote::Sandstorm));
	world.Wait(GUST_OUT_MS[0]);
	CHECK(world.X() < 8.f); // blown out of the run, into the shelter, and no farther
	CHECK(world.X() > 7.5f);
}

TEST_CASE("sandstorm: the haze closes in near a hall and the gust drowns the other sounds") {
	SimWorld world;
	load(world);
	CHECK(world.dungeon.StormHaze() == 0.f); // far from the hall
	REQUIRE(world.WalkTo(7.5f));
	waitFor(world, SpikeState::Retracted);
	world.Wait(GUST_HAZE_FADE_MS * 3);
	const float calm = world.dungeon.StormHaze();
	CHECK(calm > 0.f);
	CHECK(world.dungeon.SoundMuffle() < 0.05f);
	waitFor(world, SpikeState::Extended);
	world.Wait(GUST_OUT_MS[0] * 3 / 4);
	CHECK(world.dungeon.StormHaze() > calm);
	CHECK(world.dungeon.SoundMuffle() > 0.3f);
}

TEST_CASE("sandstorm: the checker wants each run crossed against the wind in one calm") {
	const int left = GUST_LEFT;
	CHECK(gustsCrossable(corridor({0, 0, 0}), 5, 2, 3)); // stone, 3: (2 cells + the cross) in 5 s
	CHECK_FALSE(gustsCrossable(corridor({0, 0, 0, 0, 0}), 5, 2, 5));
	CHECK(gustsCrossable(corridor({0, 0}, Structure::Sand), 5, 2, 2));
	CHECK_FALSE(gustsCrossable(corridor({0, 0, 0}, Structure::Sand), 5, 2, 3)); // slower in sand
	CHECK(gustsCrossable(corridor({2, 2, 2}, Structure::Sand), 5, 2, 3)); // the slow gusts' longer calm

	CHECK_FALSE(warns(checkLevel(corridor({left, left, left})), "sand gust"));
	CHECK(warns(checkLevel(corridor({0, 0, 0, 0, 0})), "cannot be crossed against the wind"));
	CHECK(warns(checkLevel(corridor({0, left})), "blow both ways"));
	const LevelReport ok = checkLevel(corridor({0, 0}));
	CHECK(ok.sandGusts == 2);
	CHECK(ok.pathSandGusts == 2);
}

TEST_CASE("sandstorm: the checker wants a floor under a gust and a safe place downwind") {
	LevelGrid grid = corridor({0, 0});
	Tile quick;
	quick.structure = Structure::Quicksand;
	grid.set(7, 2, quick); // just downwind of the run
	CHECK(warns(checkLevel(grid), "blow the player into quicksand"));

	grid = corridor({GUST_LEFT, GUST_LEFT});
	grid.set(4, 2, Tile{Spike, 0, 0});
	CHECK(warns(checkLevel(grid), "blow the player into Spike"));

	grid = corridor({0, 0});
	grid.set(7, 1, Tile{}); // a hole past the run
	CHECK(warns(checkLevel(grid), "blow the player into a drop"));

	grid = corridor({0, 0});
	grid.set(5, 1, Tile{}); // the gust over the hole
	CHECK(warns(checkLevel(grid), "a sand gust without a floor under it"));

	grid = corridor({0, 0});
	grid.set(8, 2, wallTile()); // rock stops the push
	CHECK_FALSE(warns(checkLevel(grid), "blow the player into"));
}

TEST_CASE("sandstorm: the desert's halls") {
	for (const int level : {33, 34, 36}) {
		LevelGrid grid;
		const std::string path = "content/levels/lvl" + std::to_string(level);
		REQUIRE(loadLevelFile(path.c_str(), grid).empty());
		const LevelReport r = checkLevel(grid);
		CHECK(r.sandGusts >= 6);
		CHECK(r.pathSandGusts > 0);
		CHECK_FALSE(warns(r, "sand gust"));
	}
}
