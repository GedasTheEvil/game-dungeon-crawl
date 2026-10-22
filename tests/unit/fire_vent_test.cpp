// Fire vents (plan fire-jet-trap): a rhythm trap on the wall spikes' cycle whose jet burns whoever stands in its cell.
#include "../../external/doctest/doctest.h"
#include "../../src/world/level_check.h"
#include "../../src/world/spike_cycle.h"
#include "sim_world.h"
#include <optional>
#include <string>
#include <vector>

namespace {
constexpr int VENT_COL = 5, ROW = 2; // tests/levels/fire_vents: a vent at col 5, a pair at cols 10-11

Tile ventCell(int phase) {
	Tile cell;
	setFixture(cell, Fixture{FireVent, static_cast<int>(SpikeSpeed::Normal), phase});
	return cell;
}

// A corridor on row 1 with the given fixtures from col 3 on: 'f' a fire vent, 's' wall spikes, at those phases.
LevelGrid corridor(const std::string& kinds, const std::vector<int>& phases) {
	LevelGrid grid;
	for (int col = 1; col < 12; col++)
		grid.set(col, 1, Tile{});
	grid.set(1, 1, Tile{Door, GateEntrance, 0});
	grid.set(11, 1, Tile{Door, GateExit, 0});
	for (size_t k = 0; k < kinds.size(); k++) {
		Tile cell;
		setFixture(cell, Fixture{kinds[k] == 'f' ? FireVent : WallSpikes, 0, phases[k]});
		grid.set(3 + static_cast<int>(k), 1, cell);
	}
	return grid;
}

bool warns(const LevelReport& r, const char* part) {
	for (const std::string& w : r.warnings)
		if (w.find(part) != std::string::npos)
			return true;
	return false;
}

SpikeState stateAt(SimWorld& world) { return spikeState(world.dungeon.Cell(VENT_COL, ROW).fixture); }

// Ticks until the vent comes into state s, at its start (at most a minute of game time).
void waitFor(SimWorld& world, SpikeState s) {
	int t = 0;
	for (; t < 60000 && stateAt(world) == s; t += UPDATE_TICK_MS)
		world.Tick();
	for (; t < 60000 && stateAt(world) != s; t += UPDATE_TICK_MS)
		world.Tick();
	REQUIRE(stateAt(world) == s);
}

// The HP the player loses standing in the vent's cell through one jet, from the jet's start to its end.
int jetLoss(SimWorld& world) {
	REQUIRE(world.Load("tests/levels/fire_vents"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(4.f));
	waitFor(world, SpikeState::Retracted);
	REQUIRE(world.WalkTo(5.5f));
	waitFor(world, SpikeState::Extended);
	const int before = world.Hp();
	waitFor(world, SpikeState::Retracted);
	return before - world.Hp();
}
} // namespace

TEST_CASE("the checker: fire vents are rhythm traps, crossed like wall spikes; they need a floor, no water") {
	CHECK_FALSE(warns(checkLevel(corridor("f", {0})), "cannot be walked through"));
	CHECK_FALSE(warns(checkLevel(corridor("ff", {0, 4})), "cannot be walked through"));
	CHECK(warns(checkLevel(corridor("ff", {0, 2})), "cannot be walked through"));
	CHECK(warns(checkLevel(corridor("fs", {0, 2})), "cannot be walked through")); // a mixed run is one run
	CHECK(warns(checkLevel(corridor("fff", {0, 0, 0})), "cannot be walked through"));
	const LevelReport r = checkLevel(corridor("f", {0}));
	CHECK(r.valid);
	CHECK(r.fireVents == 1);
	CHECK(r.pathFireVents == 1);
	CHECK(r.warnings.empty()); // no wall above needed: the jet blocks nothing

	LevelGrid floating = corridor("", {});
	floating.set(3, 1, ventCell(0));
	floating.set(3, 0, Tile{});
	CHECK(warns(checkLevel(floating), "a fire vent without a floor"));
	LevelGrid wet = corridor("", {});
	Tile water = ventCell(0);
	water.structure = Structure::HalfWater;
	wet.set(3, 1, water);
	CHECK(warns(checkLevel(wet), "a fire vent in water"));
}

TEST_CASE("the jet burns through armour: an armoured player loses as much as an unarmoured one, and burns on") {
	SimWorld bare;
	const int lossBare = jetLoss(bare);
	CHECK(lossBare > 0);
	CHECK(bare.Told("You smell your eyebrows"));
	CHECK(bare.player.burning.Any()); // leaving it late still costs a few ticks
	SimWorld armoured;
	armoured.player.stats.AddArmor(30);
	CHECK(jetLoss(armoured) == lossBare);
}

TEST_CASE("the jet blocks nothing, a jump does not clear it; off, the player walks through unhurt") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/fire_vents"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(4.f));
	waitFor(world, SpikeState::Retracted);
	REQUIRE(world.WalkTo(6.6f)); // through while it is off
	CHECK(world.Hp() == 50);
	CHECK_FALSE(world.player.burning.Any());

	REQUIRE(world.WalkTo(4.4f));
	waitFor(world, SpikeState::Extended);
	world.facing = 1;
	world.Jump(); // over the vent: into the column
	world.HoldWalk(1, 600);
	CHECK(world.X() > 5.5f); // not blocked
	CHECK(world.Hp() < 50);
	CHECK(world.player.burning.Any());
}

TEST_CASE("the trap ward and the Fire Warding amulet both cut the jet, one after the other") {
	auto lossWearing = [](int trapCut, int fireCut) {
		SimWorld world;
		AmuletBonus bonus;
		bonus.trapCutPercent = trapCut;
		bonus.resist[FIRE] = NORMAL - fireCut;
		world.player.stats.Wear(bonus, false);
		return jetLoss(world);
	};
	const int bare = lossWearing(0, 0), trapWard = lossWearing(50, 0), fireWard = lossWearing(0, 50);
	const int both = lossWearing(50, 50);
	CHECK(trapWard < bare);
	CHECK(fireWard < bare);
	CHECK(both < trapWard);
	CHECK(both < fireWard);
	CHECK(both > 0);
	SimWorld immune;
	AmuletBonus ward;
	ward.trapCutPercent = 100;
	immune.player.stats.Wear(ward, false);
	CHECK(jetLoss(immune) == 0);
	CHECK_FALSE(immune.player.burning.Any());
}

TEST_CASE("a mummy in the jet takes double a trap's damage (weak to fire), and burns; its fire kill gives no XP") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	const MonsterLinks links{&world.player, &world.journal, &world.events, 1, &world.random.gameplay};
	Monster burnt, stabbed;
	burnt.Spawn(SimWorld::monsterTypes()[static_cast<size_t>(MonsterMummy)], 3, 1, links, world.random.effects);
	stabbed.Spawn(SimWorld::monsterTypes()[static_cast<size_t>(MonsterMummy)], 4, 1, links, world.random.effects);
	for (int k = 0; k < 10; k++) {
		world.Wait(TRAP_HURT_INTERVAL_MS);
		burnt.StandInFire();
		stabbed.StandInTrap(std::nullopt, world.random.gameplay);
	}
	const int lostBurnt = burnt.MaxHealth() - burnt.Health(), lostStabbed = stabbed.MaxHealth() - stabbed.Health();
	CHECK(lostStabbed > 0);
	CHECK(lostBurnt == 2 * lostStabbed);
	CHECK(burnt.Burns());
	bool rewarded = false; // in the jet till it is nearly dead, then the fire alone finishes it
	for (int t = 0; t < 60000 && burnt.Alive(); t += UPDATE_TICK_MS) {
		world.Wait(UPDATE_TICK_MS);
		if (burnt.Health() > 3)
			burnt.StandInFire();
		rewarded = burnt.UpdateBurning() || rewarded;
	}
	CHECK_FALSE(burnt.Alive());
	CHECK_FALSE(rewarded);
}
