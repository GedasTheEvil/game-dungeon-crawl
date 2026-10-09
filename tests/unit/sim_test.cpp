#include "../../external/doctest/doctest.h"
#include "sim_world.h"

TEST_CASE("a walker stops at a wall: the rat behind it never bites") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/monster_walls"));
	world.Wait(15000);
	CHECK(world.Hp() == 50);
	CHECK(world.player.Alive());
}

TEST_CASE("a coward keeps out of an armed rock fall, a giant rat leaps over it") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/coward_rock"));
	world.player.stats.AddXP(1000, world.events);
	world.Wait(20000);
	CHECK(world.Hp() == 62);
	CHECK(world.dungeon.NearestMonsterHealth() == 12);

	REQUIRE(world.Load("tests/levels/coward_rock_leap"));
	world.Wait(6000);
	CHECK(world.Hp() < 62);
	CHECK(world.dungeon.NearestMonsterHealth() == 60);
}

TEST_CASE("the cobra rears up and spits from afar: its venom poisons the player") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/cobra"));
	REQUIRE(world.WalkTo(world.X() + 5.f));
	const int hp = world.Hp();
	for (int t = 0; t < 250 && !world.player.stats.poison.Any(); t++)
		world.Tick();
	CHECK(world.Saw(MonsterCobra, CreatureMove::Rear));
	CHECK(world.Saw(MonsterCobra, CreatureMove::Spit));
	CHECK(world.player.stats.poison.Mask() == 2); // medium
	CHECK(world.Hp() < hp);
}

TEST_CASE("Sobek charges along the row through the player and is stunned by the wall") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/sobek"));
	REQUIRE(world.WalkTo(10.8f));
	bool stunned = false;
	for (int t = 0; t < 400 && !stunned; t++) {
		world.Tick();
		stunned = world.dungeon.Boss() != nullptr && world.dungeon.Boss()->Stunned();
	}
	CHECK(world.Saw(MonsterSobek, CreatureMove::Charge));
	CHECK(stunned);
	CHECK(world.dungeon.Boss()->CentreX() < 10.8f); // ran on past the player, into the west wall
}

TEST_CASE("Apep dives into the floor, out of reach, and comes up elsewhere on the row") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/apep"));
	REQUIRE(world.WalkTo(9.5f));
	bool hidden = false;
	float downAt = 0.f;
	for (int t = 0; t < 600 && !hidden; t++) {
		world.Tick();
		const Monster* apep = world.dungeon.Boss();
		hidden = apep != nullptr && apep->Hidden();
		if (hidden)
			downAt = apep->CentreX();
	}
	REQUIRE(hidden);
	CHECK(world.Saw(MonsterApep, CreatureMove::Burrow));
	const int hp = world.dungeon.BossHealth();
	world.dungeon.AttackNearest(1000, {100, 0, 0}, 30.f, downAt < world.X() ? -1 : 1); // a minion may take it
	CHECK(world.dungeon.BossHealth() == hp);
	for (int t = 0; t < 300 && world.dungeon.Boss()->Burrowing(); t++)
		world.Tick();
	CHECK_FALSE(world.dungeon.Boss()->Burrowing());
	CHECK(world.dungeon.Boss()->CentreX() != doctest::Approx(downAt));
}

TEST_CASE("a bat leaves its roost when the player comes near, swoops through and bites") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/bats"));
	world.Wait(100);
	CHECK_FALSE(world.Saw(MonsterBat, CreatureMove::Swoop));
	REQUIRE(world.WalkTo(world.X() + 3.85f));
	world.Wait(2000);
	CHECK(world.Saw(MonsterBat, CreatureMove::Swoop));
	CHECK(world.Hp() < 50);
}
