// Crossing a spike or death trap on foot (TrapBell): a bell curve of damage over its hitbox, about SPIKES_CROSS_HP for
// a walk over the spikes and a quarter more at a sprint, a share of the max HP at high level; its edge only nicks.
#include "../../external/doctest/doctest.h"
#include "../../src/entities/trap_hurt.h"
#include "sim_world.h"

namespace {
// The HP a player of maxHp loses walking (or sprinting) from col 3 to col 6 over the trap of the level, at col 4 or 5.
int crossLoss(const char* level, int maxHp, bool sprint) {
	SimWorld world;
	world.player.stats.AddMaxHP((maxHp / 50 - 1) * 100);
	REQUIRE(world.player.stats.CurrentMaxHP() == maxHp);
	REQUIRE(world.Load(level));
	world.WaitTicks(5);
	REQUIRE(world.WalkTo(3.5f));
	world.player.stats.SetSprintRequested(sprint);
	world.WalkTo(6.6f);
	return maxHp - world.Hp();
}
} // namespace

TEST_CASE("the bell: a whole crossing is all of it, the middle weighs most, the edges little") {
	CHECK(TrapBell::share(-1.f, 1.f) == doctest::Approx(1.0));
	CHECK(TrapBell::share(1.f, -1.f) == doctest::Approx(1.0));
	CHECK(TrapBell::share(-1.f, 0.f) == doctest::Approx(0.5));
	CHECK(TrapBell::share(-0.1f, 0.1f) > 10.0 * TrapBell::share(-1.f, -0.8f));
	CHECK(TrapBell::share(-1.f, -0.8f) < 0.02);
	CHECK(TrapBell::speedFactor(WALK_SPEED) == doctest::Approx(1.0));
	CHECK(TrapBell::speedFactor(3.f * WALK_SPEED) == doctest::Approx(TRAP_SPRINT_FACTOR));
	CHECK(TrapBell::crossHundredths(200, 20, 50) == 20000);
	CHECK(TrapBell::crossHundredths(200, 20, 2000) == 40000);
}

TEST_CASE("walking over the spikes costs about 200 HP, sprinting about 250: a starting player dies") {
	const int walk = crossLoss("tests/levels/spikes_one", 5000, false); // 20% of 5000: 1000, so read it per 200
	const int sprint = crossLoss("tests/levels/spikes_one", 5000, true);
	CHECK(walk >= 1000);
	CHECK(walk <= 1050); // and the stabs while in it
	CHECK(sprint >= 1250 * 95 / 100);
	CHECK(sprint <= 1250 * 105 / 100);
	SimWorld fresh; // 50 HP
	REQUIRE(fresh.Load("tests/levels/spikes_one"));
	fresh.WaitTicks(5);
	fresh.WalkTo(6.6f);
	CHECK_FALSE(fresh.player.Alive());
	const int low = crossLoss("tests/levels/spikes_one", 400, false); // under 1000 HP the floor: 200
	CHECK(low >= SPIKES_CROSS_HP);
	CHECK(low <= SPIKES_CROSS_HP + 30);
}

TEST_CASE("the death trap costs twice the spikes") {
	const int walk = crossLoss("tests/levels/death_one", 5000, false); // 40% of 5000
	CHECK(walk >= 2000);
	CHECK(walk <= 2700); // and its strong poison while in it (5% a second)
}

TEST_CASE("a step into the spikes' edge and back only nicks: a warning, no kill") {
	SimWorld world; // 50 HP
	REQUIRE(world.Load("tests/levels/spikes_one"));
	world.WaitTicks(5);
	REQUIRE(world.WalkTo(4.21f)); // the hitbox starts at 4.18, the box (plan trap-player-box) touches it from 4.135
	REQUIRE(world.WalkTo(3.5f));
	CHECK(world.Hp() < 50);
	CHECK(world.Hp() >= 40);
}
