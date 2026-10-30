// Spike and death traps touch the player's box, not the centre point, less TRAP_BOX_FORGIVE of its half width on
// each side (plan trap-player-box); a reckless walker's box too.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
constexpr float SPIKES_LEFT = 4.5f - TRAP_HITBOX_X_SCALE * SPIKES_SCALE; // tests/levels/spikes_one: col 4

// The HP the player (50) has after walking from the entrance to x (left of the spikes) and standing there a second.
int hpAt(float x) {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/spikes_one"));
	world.WaitTicks(5);
	REQUIRE(world.WalkTo(x));
	REQUIRE(world.X() < SPIKES_LEFT);
	world.Wait(1000);
	return world.Hp();
}
} // namespace

TEST_CASE("the player's feet in the spikes hurt with the centre still outside them") {
	SimWorld world;
	const float body = world.player.HalfWidth() * (1.f - TRAP_BOX_FORGIVE);
	REQUIRE(body > 3.f * PLAYER_MOVE_STEP); // room for the steps below
	CHECK(hpAt(SPIKES_LEFT - body + PLAYER_MOVE_STEP) < 50);
}

TEST_CASE("grazing the spikes with the box's outer quarter does not hurt") {
	SimWorld world;
	const float half = world.player.HalfWidth();
	const float forgiven = half * TRAP_BOX_FORGIVE;
	REQUIRE(forgiven > PLAYER_MOVE_STEP);
	// The box's edge over the spikes' edge, by less than its forgiven quarter (WalkTo overshoots by a step at most).
	CHECK(hpAt(SPIKES_LEFT - half + forgiven - PLAYER_MOVE_STEP) == 50);
	CHECK(hpAt(SPIKES_LEFT - half - PLAYER_MOVE_STEP) == 50);
}

TEST_CASE("a reckless mummy's box takes the spikes before its centre gets there") {
	SimWorld world; // tests/levels/reckless_spikes: the mummy (col 5) walks left into the spikes (col 4)
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/reckless_spikes"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(3.95f));
	const Monster* mummy = world.dungeon.NearestMonster();
	REQUIRE(mummy != nullptr);
	const int full = world.Nearest();
	for (int t = 0; t < 1000 && world.Nearest() == full; t++)
		world.Tick();
	REQUIRE(world.Nearest() < full);
	const float spikesRight = 4.5f + TRAP_HITBOX_X_SCALE * SPIKES_SCALE;
	CHECK(mummy->CentreX() > spikesRight);
	CHECK(mummy->CentreX() <= spikesRight + mummy->HalfWidth() * (1.f - TRAP_BOX_FORGIVE));
}
