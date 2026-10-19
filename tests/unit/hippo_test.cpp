// The hippo (plan hippo): lies under the water, comes out charging along its row, a wall stuns it; slow on land, fast
// in the water.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
const Monster* hippo(const SimWorld& world) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == MonsterHippo)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

// tests/levels/hippo: the hippo in col 14 of a pool (cols 10-19), the player in col 8 by the west wall. From col 9
// on it is in the view window (it appears), still out of its charge range. A player who lives through a charge.
const Monster* loadPool(SimWorld& world) {
	REQUIRE(world.Load("tests/levels/hippo"));
	world.player.stats.AddXP(1000000, world.events);
	REQUIRE(world.WalkTo(9.2f));
	world.Tick();
	return hippo(world);
}

// Ticks until the hippo is stunned, at most 4 s.
bool waitStunned(SimWorld& world, const Monster& h) {
	for (int t = 0; t < 4000 / UPDATE_TICK_MS; t++) {
		if (h.Stunned())
			return true;
		world.Tick();
	}
	return h.Stunned();
}
} // namespace

TEST_CASE("the hippo lies still under the water while the player is out of its charge range") {
	SimWorld world;
	const Monster* h = loadPool(world);
	REQUIRE(h != nullptr);
	const float x = h->CentreX();
	REQUIRE(x - world.X() > HIPPO_CHARGE_MAX);
	world.Wait(3000);
	CHECK_FALSE(h->Alerted());
	CHECK(h->InWater());
	CHECK(h->CentreX() == doctest::Approx(x));
}

TEST_CASE("the hippo comes out charging along the row, through the player, and a wall stuns it") {
	SimWorld world;
	const Monster* h = loadPool(world);
	REQUIRE(h != nullptr);
	const int hp = world.Hp();
	REQUIRE(world.WalkTo(10.8f)); // in the water: no jump over the charge
	CHECK(h->Alerted());
	CHECK(world.Saw(MonsterHippo, CreatureMove::Charge));
	REQUIRE(waitStunned(world, *h));
	CHECK(world.Hp() <= hp - h->Type()->damage * CHARGE_HIT_FACTOR / 2); // a charge, through any resistances
	CHECK(h->CentreX() < 10.8f); // ran on past the player, into the west wall
}

TEST_CASE("the hippo walks slowly on land and swims fast in half water") {
	SimWorld world;
	const Monster* h = loadPool(world);
	REQUIRE(h != nullptr);
	REQUIRE(h->InWater());
	const float swim = h->WalkStep();
	REQUIRE(world.WalkTo(10.8f));
	REQUIRE(waitStunned(world, *h));
	REQUIRE_FALSE(h->InWater()); // on the dry floor by the west wall
	const float walk = h->WalkStep();
	CHECK(swim >= 2.5f * walk);
	CHECK(h->Type()->speed < monsterKind(MonsterCrocodile)->speed); // slower than the crocodile on land
}
