// Crouch (plan crouch): the crouch key held lowers the player under a crawl space, at a third of the walk speed, with
// no sprint, jump, climb or attack; under the low ceiling they stay down with the key up. Crouched, a high swing and a
// shot aimed at the standing chest miss, a low bite hits. The poses: tests/scenarios/solved/crouch.txt.

#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
// tests/levels/subcells, row 2: the exit at col 24, a crawl space a quarter high at cols 25-26, open floor at col 27.
constexpr int ROW = 2;

void crouch(SimWorld& world, bool down) {
	world.player.crouchHeld = down;
	world.Tick();
}

// Tiles walked in ms with the walk key held to the right.
float walked(SimWorld& world, int ms) {
	const float from = world.X();
	world.HoldWalk(1, ms);
	return world.X() - from;
}
} // namespace

TEST_CASE("crouched, the player passes a crawl space, cannot stand up under it, and stands up past it") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/subcells"));
	REQUIRE(world.dungeon.PlacePlayer(24, ROW));
	world.Tick();
	CHECK_FALSE(world.WalkTo(25.5f)); // standing: the opening stops them

	crouch(world, true);
	REQUIRE(world.player.crouched);
	CHECK(world.player.Height() == doctest::Approx(CROUCH_HEIGHT_FACTOR * world.player.StandingHeight()));
	REQUIRE(world.WalkTo(25.6f));
	crouch(world, false); // the key up under the ceiling: still down
	CHECK(world.player.crouched);
	world.Wait(500);
	CHECK(world.player.crouched);
	REQUIRE(world.WalkTo(26.5f)); // on at the crouched pace
	CHECK(world.player.crouched);
	REQUIRE(world.WalkTo(27.5f)); // out from under it: up again
	world.Tick();
	CHECK_FALSE(world.player.crouched);
	CHECK(world.player.Height() == doctest::Approx(world.player.StandingHeight()));
}

TEST_CASE("the crouched walk is a third of the walk speed, with no sprint and no stamina spent") {
	SimWorld standing;
	REQUIRE(standing.Load("tests/levels/empty_row"));
	const float walk = walked(standing, 1000);
	REQUIRE(walk > 0.5f);

	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	crouch(world, true);
	REQUIRE(world.player.crouched);
	CHECK(walked(world, 1000) == doctest::Approx(walk / 3.f).epsilon(0.05));

	const int stamina = world.player.stats.Stamina();
	world.player.stats.SetSprintRequested(true); // shift down: crouched, it does not sprint
	CHECK(walked(world, 1000) == doctest::Approx(walk / 3.f).epsilon(0.05));
	CHECK(world.player.stats.Stamina() >= stamina);
}

TEST_CASE("crouched, no jump and no attack; up again, both work") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	world.Equip(ItemKind::Club);
	world.Wait(1100); // the attack timer of the load
	crouch(world, true);
	world.Jump();
	world.Tick();
	CHECK_FALSE(world.player.jump.jumping);
	CHECK_FALSE(world.Attack());
	world.Tick();
	CHECK(world.Told("Stand up to strike"));

	crouch(world, false);
	CHECK_FALSE(world.player.crouched);
	world.Wait(1100); // SimWorld::Attack's timer check restarted the timer
	CHECK(world.Attack());
	world.Wait(1500);
	world.Jump();
	world.Tick();
	CHECK(world.player.jump.jumping);
}

TEST_CASE("a crouch starts only on the floor: not in a jump") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	world.Jump();
	crouch(world, true);
	CHECK_FALSE(world.player.crouched);
	world.Wait(1500); // landed, the key still down
	CHECK(world.player.crouched);
}

TEST_CASE("crouched, a high swing misses, a low bite and a body blow hit") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	const MonsterLinks links{&world.player, &world.journal, &world.events, 1, &world.random.gameplay};
	const auto row = static_cast<float>(1); // tests/levels/empty_row: the player's row
	for (int kind :
		 {MonsterMummy, MonsterAnubis, MonsterAnubisBoss, MonsterUshabti, MonsterRat, MonsterCrocodile, MonsterHippo}) {
		const MonsterType& type = SimWorld::monsterTypes()[static_cast<size_t>(kind)];
		CAPTURE(type.label);
		Monster mon;
		mon.Spawn(type, 3, 1, links, world.random.effects);
		const float px = mon.Right() + MONSTER_BITE_REACH + world.player.HalfWidth();
		world.player.crouched = false;
		CHECK(mon.Reaches(px, row, 1));
		world.player.crouched = true;
		CHECK(mon.Reaches(px, row, 1) == (type.melee.height != AttackHeight::High));
	}
	CHECK(monsterKind(MonsterMummy)->melee.height == AttackHeight::High);
	CHECK(monsterKind(MonsterRat)->melee.height == AttackHeight::Low);
}

TEST_CASE("crouched, a shot aimed at the standing chest flies over; one at the feet hits") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	const float px = world.X(), py = world.Y();
	const float chest = py + SHOT_CHEST * world.player.StandingHeight();
	CHECK(world.player.Covers(px, py, px, chest));
	crouch(world, true);
	REQUIRE(world.player.crouched);
	CHECK_FALSE(world.player.Covers(px, py, px, chest));
	CHECK(world.player.Covers(px, py, px, py + 0.01f));
	CHECK(world.player.Covers(px, py, px, py + SHOT_CHEST * world.player.Height())); // aimed at the crouched chest
}

TEST_CASE("the down key crouches off a ladder; at the top of one it climbs down instead") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/climb_down"));
	REQUIRE(world.WalkTo(6.45f)); // the ladder's top cell (tests/unit/climb_test.cpp)
	const float top = world.Y();
	crouch(world, true);
	CHECK_FALSE(world.player.crouched);
	REQUIRE(world.Climb(-0.5f));
	CHECK(world.Y() < top);
	CHECK_FALSE(world.player.crouched);
}
