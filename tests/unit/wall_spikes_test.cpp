// Wall spikes (plan rhythm-traps): the cycle, the checker's walk through a run of them, and the trap in the world.
#include "../../external/doctest/doctest.h"
#include "../../src/world/level_check.h"
#include "../../src/world/spike_cycle.h"
#include "../../src/world/tile_defs.h"
#include "sim_world.h"
#include <string>
#include <vector>

namespace {
Fixture spikes(SpikeSpeed speed, int phase) { return Fixture{WallSpikes, static_cast<int>(speed), phase}; }
Tile spikesCell(SpikeSpeed speed, int phase) {
	Tile cell;
	setFixture(cell, spikes(speed, phase));
	return cell;
}

// A corridor on row 1 with the wall spikes from col 3 on, the given phases, all at speed.
LevelGrid corridor(const std::vector<int>& phases, SpikeSpeed speed = SpikeSpeed::Normal) {
	LevelGrid grid;
	for (int col = 1; col < 12; col++)
		grid.set(col, 1, Tile{});
	grid.set(1, 1, Tile{Door, GateEntrance, 0});
	grid.set(11, 1, Tile{Door, GateExit, 0});
	for (size_t k = 0; k < phases.size(); k++)
		grid.set(3 + static_cast<int>(k), 1, spikesCell(speed, phases[k]));
	return grid;
}

bool warnsUncrossable(const LevelReport& r) {
	for (const std::string& w : r.warnings)
		if (w.find("cannot be walked through") != std::string::npos)
			return true;
	return false;
}

SpikeState stateAt(SimWorld& world, int col, int row) { return spikeState(world.dungeon.Cell(col, row).fixture); }

// Ticks until the wall spikes at (col, row) come into state s (at most a minute of game time): at its start, whatever
// the clock's phase (the virtual clock runs on from the tests before).
void waitFor(SimWorld& world, int col, int row, SpikeState s) {
	int t = 0;
	for (; t < 60000 && stateAt(world, col, row) == s; t += UPDATE_TICK_MS)
		world.Tick();
	for (; t < 60000 && stateAt(world, col, row) != s; t += UPDATE_TICK_MS)
		world.Tick();
	REQUIRE(stateAt(world, col, row) == s);
}
} // namespace

TEST_CASE("the wall spikes' cycle: out first, then in, the telegraph just before they come out again") {
	const Fixture t = spikes(SpikeSpeed::Normal, 0);
	CHECK(spikeStateAt(t, 0) == SpikeState::Extended);
	CHECK(spikeStateAt(t, SPIKES_OUT_MS[0] - 1) == SpikeState::Extended);
	CHECK(spikeStateAt(t, SPIKES_OUT_MS[0]) == SpikeState::Retracted);
	CHECK(spikeStateAt(t, SPIKES_PERIOD_MS[0] - SPIKES_TELEGRAPH_MS - 1) == SpikeState::Retracted);
	CHECK(spikeStateAt(t, SPIKES_PERIOD_MS[0] - SPIKES_TELEGRAPH_MS) == SpikeState::Telegraph);
	CHECK(spikeStateAt(t, SPIKES_PERIOD_MS[0]) == SpikeState::Extended);
	// Half a cycle later: phase 4 is in when phase 0 is out.
	const Fixture half = spikes(SpikeSpeed::Normal, SPIKE_PHASES / 2);
	CHECK(spikeStateAt(half, 0) == SpikeState::Retracted);
	CHECK(spikeStateAt(half, SPIKES_PERIOD_MS[0] / 2) == SpikeState::Extended);
	// The live state in the value keeps the phase.
	Tile live;
	setFixture(live, half);
	setSpikeState(live.fixture, SpikeState::Extended);
	CHECK(spikePhase(live.fixture) == SPIKE_PHASES / 2);
	CHECK(isSolidTile(live));
	setSpikeState(live.fixture, SpikeState::Telegraph);
	CHECK_FALSE(isSolidTile(live));
	// The safe windows of two traps: in, both, from the first one's retract to the second one's coming out.
	CHECK(spikesInBetween(t, SPIKES_OUT_MS[0], SPIKES_PERIOD_MS[0] / 2 - 1));
	CHECK_FALSE(spikesInBetween(half, SPIKES_OUT_MS[0], SPIKES_PERIOD_MS[0] / 2));
}

TEST_CASE("the checker: wall spikes the player can walk through pass, a run whose safe windows do not line up warns") {
	CHECK_FALSE(warnsUncrossable(checkLevel(corridor({0}))));
	CHECK_FALSE(warnsUncrossable(checkLevel(corridor({0, 0}))));
	CHECK_FALSE(
		warnsUncrossable(checkLevel(corridor({0, 4})))
	); // alternate: each way, one goes in as the other comes out
	CHECK(warnsUncrossable(checkLevel(corridor({0, 2}))));
	CHECK(warnsUncrossable(checkLevel(corridor({0, 0, 0})))); // three take longer to walk through than they stay in
	CHECK_FALSE(warnsUncrossable(checkLevel(corridor({0, 1}, SpikeSpeed::Slow))));
	const LevelReport r = checkLevel(corridor({0}));
	CHECK(r.valid); // never solid in level data: no softlock
	CHECK(r.wallSpikes == 1);
	CHECK(r.pathWallSpikes == 1);
	CHECK(r.warnings.empty());
}

TEST_CASE("the checker: wall spikes need a floor under them and a wall above them") {
	LevelGrid grid = corridor({0});
	grid.set(3, 2, Tile{});
	const LevelReport r = checkLevel(grid);
	bool above = false;
	for (const std::string& w : r.warnings)
		above = above || w.find("without a wall above") != std::string::npos;
	CHECK(above);
}

TEST_CASE(
	"wall spikes out block like a wall, a jump too; in, the player walks through; coming out, they stab and "
	"shove"
) {
	SimWorld world; // tests/levels/wall_spikes: the trap at col 5, row 2
	REQUIRE(world.Load("tests/levels/wall_spikes"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(4.6f));
	waitFor(world, 5, 2, SpikeState::Extended);
	world.HoldWalk(1, 300); // a wall
	CHECK(world.X() < 5.f - PLAYER_BODY_HALF_WIDTH + 0.01f);
	world.facing = 1;
	world.Jump(); // no clearance: the spikes fill the corridor
	world.HoldWalk(1, 500);
	CHECK(world.X() < 5.f);
	CHECK(world.Hp() == 50);

	waitFor(world, 5, 2, SpikeState::Retracted);
	REQUIRE(world.WalkTo(6.6f)); // through while they are in
	CHECK(world.Hp() == 50);
	CHECK(world.player.stats.poison.Mask() == 0);

	waitFor(world, 5, 2, SpikeState::Extended);
	waitFor(world, 5, 2, SpikeState::Retracted);
	REQUIRE(world.WalkTo(5.6f)); // into the cell, and wait there
	waitFor(world, 5, 2, SpikeState::Extended);
	CHECK(world.Told("Spikes shoot out of the wall"));
	CHECK(world.Hp() == 50 - SPIKES_STAB_DAMAGE);
	CHECK(world.player.stats.poison.Mask() == 1); // weak
	CHECK(world.X() > 6.f + PLAYER_BODY_HALF_WIDTH); // shoved out on the near side
	world.Wait(SPIKES_OUT_MS[0]);
	CHECK(world.Hp() > 50 - SPIKES_STAB_DAMAGE - 5); // one stab, then only the weak poison
}

TEST_CASE("the trap ward wards off the wall spikes' stab and its poison") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/wall_spikes"));
	world.WaitTicks(30);
	world.player.stats.Wear(
		[] {
			AmuletBonus ward;
			ward.trapCutPercent = 100;
			return ward;
		}(),
		false
	);
	REQUIRE(world.WalkTo(4.f));
	waitFor(world, 5, 2, SpikeState::Retracted);
	REQUIRE(world.WalkTo(5.4f));
	waitFor(world, 5, 2, SpikeState::Extended);
	CHECK(world.Hp() == 50);
	CHECK(world.player.stats.poison.Mask() == 0);
	CHECK(world.X() < 5.f - PLAYER_BODY_HALF_WIDTH); // shoved out all the same
}

TEST_CASE("a monster standing in the wall spikes as they come out is stabbed and shoved out") {
	SimWorld world; // tests/levels/wall_spikes_mummy: the trap at col 5, a mummy at col 6
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/wall_spikes_mummy"));
	world.WaitTicks(30);
	waitFor(world, 5, 2, SpikeState::Retracted);
	REQUIRE(world.WalkTo(5.4f)); // wakes it
	REQUIRE(world.WalkTo(4.7f)); // as near as the spikes let the player: it comes into their cell
	world.Wait(8000);
	const Monster* mummy = world.dungeon.NearestMonster();
	REQUIRE(mummy != nullptr);
	CHECK(mummy->Health() < mummy->MaxHealth());
	waitFor(world, 5, 2, SpikeState::Extended);
	world.Tick();
	CHECK((mummy->Right() <= 5.f || mummy->Left() >= 6.f));
}
