// Fire vents (plan fire-jet-trap): a rhythm trap on the wall spikes' cycle whose jet burns whoever stands in its cell.
#include "../../external/doctest/doctest.h"
#include "../../src/world/level_check.h"
#include "../../src/world/spike_cycle.h"
#include "sim_world.h"
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

// A player with 2000 HP (50 x 40): one jet does not kill them.
void toughen(SimWorld& world) {
	world.player.stats.AddMaxHP(3900);
	REQUIRE(world.player.stats.CurrentMaxHP() == 2000);
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
	toughen(bare);
	const int lossBare = jetLoss(bare);
	CHECK(lossBare > 0);
	CHECK(bare.Told("You smell your eyebrows"));
	CHECK(bare.player.burning.Any()); // leaving it late still costs a few ticks
	SimWorld armoured;
	toughen(armoured);
	armoured.player.stats.AddArmor(30);
	CHECK(jetLoss(armoured) == lossBare);
}

TEST_CASE("a whole jet costs 500 HP and more: at least FIRE_JET_HP_PER_SECOND, half the max HP a second") {
	SimWorld low; // 50 HP
	jetLoss(low);
	CHECK_FALSE(low.player.Alive());
	SimWorld high;
	high.player.stats.AddMaxHP(1900); // 50 -> 1000
	REQUIRE(high.player.stats.CurrentMaxHP() == 1000);
	CHECK(jetLoss(high) >= 1000 * FIRE_JET_PERCENT_PER_SECOND / 100 * SPIKES_OUT_MS[0] / 1000 * 9 / 10);
	high.Wait(BURN_MS);
	CHECK(high.Hp() < 250); // and the burning after
	SimWorld tough;
	toughen(tough); // 2000 HP: 50% a second for the 1.2 s of the jet
	CHECK(jetLoss(tough) >= 2000 * FIRE_JET_PERCENT_PER_SECOND / 100 * SPIKES_OUT_MS[0] / 1000 * 9 / 10);
	CHECK(FIRE_JET_HP_PER_SECOND * SPIKES_OUT_MS[0] / 1000 >= 500); // a jet's floor
}

TEST_CASE("the jet blocks nothing, a jump does not clear it; off, the player walks through unhurt") {
	SimWorld world;
	toughen(world);
	REQUIRE(world.Load("tests/levels/fire_vents"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(4.f));
	waitFor(world, SpikeState::Retracted);
	REQUIRE(world.WalkTo(6.6f)); // through while it is off
	CHECK(world.Hp() == 2000);
	CHECK_FALSE(world.player.burning.Any());

	REQUIRE(world.WalkTo(4.4f));
	waitFor(world, SpikeState::Extended);
	world.facing = 1;
	world.Jump(); // over the vent: into the column
	world.HoldWalk(1, 600);
	CHECK(world.X() > 5.5f); // not blocked
	CHECK(world.Hp() < 2000 - FIRE_JET_HP_PER_SECOND / 4);
	CHECK(world.player.burning.Any());
}

TEST_CASE("the trap ward and the Fire Warding amulet both cut the jet, one after the other") {
	auto lossWearing = [](int trapCut, int fireCut) {
		SimWorld world;
		toughen(world);
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

TEST_CASE("a mummy in the jet takes double (weak to fire), and burns; its fire kill gives no XP") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	const MonsterLinks links{&world.player, &world.journal, &world.events, 1, &world.random.gameplay};
	Monster burnt;
	burnt.Spawn(SimWorld::monsterTypes()[static_cast<size_t>(MonsterMummy)], 3, 1, links, world.random.effects);
	REQUIRE(burnt.Type()->trapDamagePct == 50);
	REQUIRE(burnt.Type()->resist[FIRE] == WEAK);
	for (int k = 0; k < 10; k++) {
		world.Wait(TRAP_HURT_INTERVAL_MS);
		burnt.StandInFire();
	}
	const int lostBurnt = burnt.MaxHealth() - burnt.Health();
	// Ten hits of the floor rate's tenth of a second, at the mummy's trap share (50%), weak to fire (200%).
	CHECK(lostBurnt ==
		  10 * FIRE_JET_MONSTER_HP_PER_SECOND * TRAP_HURT_INTERVAL_MS / 10 * 50 / 100 * WEAK / NORMAL / 100);
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

TEST_CASE("fire in water: a monster or the player in water takes a tenth of a fire hit (plan fire-in-water)") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	const MonsterLinks links{&world.player, &world.journal, &world.events, 1, &world.random.gameplay};
	const DamageMix flame = {0, 0, 0, 100}, bolt = {40, 0, 20, 40};
	const auto lost = [&](bool water, const DamageMix& mix) {
		Monster mon;
		mon.Spawn(SimWorld::monsterTypes()[static_cast<size_t>(MonsterMummy)], 3, 1, links, world.random.effects);
		mon.SetInWater(water, false, 0.f);
		const int before = mon.Health();
		mon.TakeWeaponHit(10, mix);
		return before - mon.Health();
	};
	CHECK(lost(false, flame) == 20); // weak to fire: 200%
	CHECK(lost(true, flame) == 2); // a tenth of that
	// Mixed: only the fire share (40%) drops; the mummy's blunt 50%, pierce 25% stay.
	CHECK(lost(false, bolt) == std::lround(10 * (40 * 50 + 20 * 25 + 40 * 200) / 10000.0));
	CHECK(lost(true, bolt) == std::lround(10 * (40 * 50 + 20 * 25 + 40 * 20) / 10000.0));
	world.player.UpdateBurning(true, world.events); // wading
	const int hp = world.player.stats.CurrentHP();
	world.player.TakeHit(20, flame, world.events);
	CHECK(hp - world.player.stats.CurrentHP() == 2);
}
