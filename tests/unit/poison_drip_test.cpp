// Poison drips (plan poison-drip-trap): a puddle that always poisons (weak), a lump of venom that falls into it on the
// wall spikes' cycle (a small hit and medium poison).
#include "../../external/doctest/doctest.h"
#include "../../src/world/climb_path.h"
#include "../../src/world/level_check.h"
#include "../../src/world/spike_cycle.h"
#include "sim_world.h"
#include <string>
#include <vector>

namespace {
constexpr int DRIP_COL = 5, ROW = 2; // tests/levels/poison_drips: a drip at col 5, a staggered run at cols 10, 12, 14

Tile dripCell(int phase) {
	Tile cell;
	setFixture(cell, Fixture{PoisonDrip, static_cast<int>(SpikeSpeed::Normal), phase});
	return cell;
}

// A corridor on row 1 under a ceiling, drips from col 3 on at those phases; an antidote chest at col 10 unless bare.
LevelGrid corridor(const std::vector<int>& phases, bool antidote = true) {
	LevelGrid grid;
	for (int col = 1; col < 12; col++)
		grid.set(col, 1, Tile{});
	grid.set(1, 1, Tile{Door, GateEntrance, 0});
	grid.set(11, 1, Tile{Door, GateExit, 0});
	if (antidote)
		grid.set(10, 1, Tile{Treasure, 3, 7});
	for (size_t k = 0; k < phases.size(); k++)
		grid.set(3 + static_cast<int>(k), 1, dripCell(phases[k]));
	return grid;
}

bool warns(const LevelReport& r, const char* part) {
	for (const std::string& w : r.warnings)
		if (w.find(part) != std::string::npos)
			return true;
	return false;
}

SpikeState stateAt(SimWorld& world) { return spikeState(world.dungeon.Cell(DRIP_COL, ROW).fixture); }

// Ticks until the drip comes into state s, at its start (at most a minute of game time).
void waitFor(SimWorld& world, SpikeState s) {
	int t = 0;
	for (; t < 60000 && stateAt(world) == s; t += UPDATE_TICK_MS)
		world.Tick();
	for (; t < 60000 && stateAt(world) != s; t += UPDATE_TICK_MS)
		world.Tick();
	REQUIRE(stateAt(world) == s);
}

bool weak(const SimWorld& world) { return world.player.stats.poison.Running(PoisonTier::Weak); }
bool medium(const SimWorld& world) { return world.player.stats.poison.Running(PoisonTier::Medium); }

// On the level, beside the drip at col 5 as the venom stops falling.
void besideTheDrip(SimWorld& world) {
	REQUIRE(world.Load("tests/levels/poison_drips"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(4.f));
	waitFor(world, SpikeState::Retracted);
}

// Lands from a jump started now.
void land(SimWorld& world) {
	for (int t = 0; t < 3000 && (world.player.jump.jumping || world.player.jump.falling); t += UPDATE_TICK_MS)
		world.HoldWalk(world.facing, UPDATE_TICK_MS);
}
} // namespace

TEST_CASE("the checker: a poison drip's puddle is a floor trap, jumped over; the drop is a rhythm trap") {
	const LevelReport r = checkLevel(corridor({0}));
	CHECK(r.valid);
	CHECK(r.poisonDrips == 1);
	CHECK(r.pathJumps == 1); // over the puddle, not through it
	CHECK(r.pathPoisonDrips == 0);
	CHECK(r.warnings.empty());
	CHECK(r.softlockCells == 0);
	CHECK_FALSE(warns(checkLevel(corridor({0, 4})), "cannot be walked through"));
	CHECK(warns(checkLevel(corridor({0, 2})), "cannot be walked through"));
	CHECK(warns(checkLevel(corridor({0, 0, 0})), "cannot be walked through"));

	CHECK(warns(checkLevel(corridor({0}, false)), "poison drips but no antidote in reach"));
	LevelGrid open = corridor({0});
	open.set(3, 2, Tile{});
	CHECK(warns(checkLevel(open), "a poison drip without a ceiling over it"));
	LevelGrid floating = corridor({0});
	floating.set(3, 0, Tile{});
	CHECK(warns(checkLevel(floating), "a poison drip without a floor under it"));
	LevelGrid ladder = corridor({0});
	Tile both = dripCell(0);
	both.type = Ladder;
	ladder.set(3, 1, both);
	CHECK(warns(checkLevel(ladder), "Ladder in a poison drip's puddle"));
}

TEST_CASE("the puddle poisons on entry at any time, weak, no damage, once per entry") {
	SimWorld world;
	besideTheDrip(world);
	const int hp = world.Hp();
	REQUIRE(world.WalkTo(5.5f)); // into it while nothing falls
	CHECK(weak(world));
	CHECK_FALSE(medium(world));
	CHECK(world.Hp() == hp);
	CHECK(world.player.stats.run.poisoned == 1);
	waitFor(world, SpikeState::Telegraph); // standing in it: no more
	CHECK(world.player.stats.run.poisoned == 1);

	REQUIRE(world.WalkTo(4.8f)); // out, and back in in the next telegraph
	waitFor(world, SpikeState::Telegraph);
	REQUIRE(world.WalkTo(5.3f));
	REQUIRE(stateAt(world) == SpikeState::Telegraph);
	CHECK(world.player.stats.run.poisoned == 2);
}

TEST_CASE("the drop hits only on the beat: a small hit and medium poison, once per drop") {
	SimWorld world;
	besideTheDrip(world);
	REQUIRE(world.WalkTo(5.5f));
	const int poisoned = world.player.stats.run.poisoned;
	waitFor(world, SpikeState::Telegraph);
	CHECK_FALSE(medium(world)); // nothing falls before the beat
	const int hp = world.Hp();
	waitFor(world, SpikeState::Extended);
	CHECK(hp - world.Hp() == DRIP_DROP_DAMAGE);
	CHECK(medium(world));
	CHECK(world.Told("saving that for you"));
	waitFor(world, SpikeState::Retracted);
	CHECK(world.player.stats.run.poisoned == poisoned + 1); // one drop, one poisoning
}

TEST_CASE("a jump clears the puddle, not the drop") {
	SimWorld world;
	besideTheDrip(world);
	world.facing = 1;
	world.Jump(); // between drops: clean over it
	land(world);
	REQUIRE(world.X() > 5.5f + DRIP_PUDDLE_HALF_WIDTH);
	CHECK_FALSE(world.player.stats.poison.Any());

	REQUIRE(world.WalkTo(7.f));
	world.facing = -1;
	waitFor(world, SpikeState::Telegraph);
	world.Wait(SPIKES_TELEGRAPH_MS - 200);
	const int hp = world.Hp();
	world.Jump(); // into the falling venom
	land(world);
	CHECK(world.X() < 5.5f - DRIP_PUDDLE_HALF_WIDTH);
	CHECK(medium(world));
	CHECK_FALSE(weak(world)); // the puddle stayed under the jump
	CHECK(world.Hp() < hp);
}

TEST_CASE("the trap ward cuts the drop's hit, never its poison") {
	SimWorld world;
	AmuletBonus ward;
	ward.trapCutPercent = 100;
	world.player.stats.Wear(ward, false);
	besideTheDrip(world);
	REQUIRE(world.WalkTo(5.f)); // under the drop's edge, out of the puddle
	CHECK_FALSE(weak(world));
	const int hp = world.Hp();
	waitFor(world, SpikeState::Extended);
	CHECK(world.Hp() == hp);
	CHECK(medium(world));
}

TEST_CASE("monsters: cowards keep out of the puddle, the reckless are poisoned once per entry, no XP for the kill") {
	LevelGrid grid = corridor({0});
	CHECK(walkerBlockedAt(grid.cells, 3, 1));
	CHECK_FALSE(walkerBlockedAt(grid.cells, 3, 1, true));

	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	const MonsterLinks links{&world.player, &world.journal, &world.events, 1, &world.random.gameplay};
	Monster mon;
	mon.Spawn(SimWorld::monsterTypes()[static_cast<size_t>(MonsterCrocodile)], 3, 1, links, world.random.effects);
	REQUIRE(mon.Type()->poisonResistPercent == 0);
	mon.StandInDrip(true, false, world.random.gameplay);
	CHECK(mon.PoisonMask() == 1); // weak
	CHECK(mon.Health() == mon.MaxHealth()); // the puddle does no damage
	const int weakMs = POISON_TIERS[static_cast<size_t>(PoisonTier::Weak)].durationMs;
	for (int t = 0; t <= weakMs; t += UPDATE_TICK_MS) { // standing in it: not again, it wears off
		mon.StandInDrip(true, false, world.random.gameplay);
		mon.UpdatePoison();
	}
	CHECK_FALSE(mon.Poisoned());
	const int hp = mon.Health();
	mon.StandInDrip(true, true, world.random.gameplay); // the drop
	CHECK(mon.PoisonMask() == 2); // medium
	CHECK(hp - mon.Health() == DRIP_DROP_DAMAGE * mon.Type()->trapDamagePct / 100);
	mon.StandInDrip(false, false, world.random.gameplay);
	mon.StandInDrip(true, false, world.random.gameplay); // back in
	CHECK(mon.PoisonMask() == 3);

	Monster rat; // the drop's poison kills it: no reward
	rat.Spawn(SimWorld::monsterTypes()[static_cast<size_t>(MonsterRat)], 3, 1, links, world.random.effects);
	rat.StandInDrip(false, true, world.random.gameplay);
	bool rewarded = false;
	for (int t = 0; t < 60000 && rat.Alive(); t += UPDATE_TICK_MS)
		rewarded = rat.UpdatePoison() || rewarded;
	CHECK_FALSE(rat.Alive());
	CHECK_FALSE(rewarded);
}
