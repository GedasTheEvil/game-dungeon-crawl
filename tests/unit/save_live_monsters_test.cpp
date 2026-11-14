// A save keeps the live monsters' health and the boss fight (plan save-live-monsters): a quick save and load in a
// fight neither heals a hurt monster nor brings back the minions the player has killed.
#include <cmath>
#include <sstream>

#include "../../external/doctest/doctest.h"
#include "sim_world.h"

TEST_CASE("a hurt monster keeps its health through a save and load") {
	SimWorld world; // tests/levels/hitbox_rat: a rat walks up to the player
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/hitbox_rat"));
	REQUIRE(world.Walk(1.f));
	world.Wait(8000);
	REQUIRE(world.Nearest() == 12);
	world.dungeon.AttackNearest(5, {100, 0, 0}, 1.f, 1);
	const int hurt = world.Nearest();
	REQUIRE(hurt > 0);
	REQUIRE(hurt < 12);
	world.SaveAndLoad();
	world.Wait(8000);
	CHECK(world.Nearest() == hurt);
	world.SaveAndLoad(); // the health is not kept once the tile spawned it: saved again, from the monster
	world.Wait(8000);
	CHECK(world.Nearest() == hurt);
}

TEST_CASE("a boss keeps its health and its fight through a save and load") {
	SimWorld world; // tests/levels/babi_hall: Babi at col 6, baboons dig out round him
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/babi_hall"));
	world.player.stats.AddXP(1000000, world.events);
	const auto row = static_cast<int>(std::floor(world.Y()));
	REQUIRE(world.dungeon.SpawnMonster(6, row));
	world.Tick();
	const int full = world.dungeon.BossHealth();
	REQUIRE(full > 0);
	REQUIRE(world.dungeon.LivingMinions() > 0);
	world.dungeon.HurtBoss(full * 9 / 10);
	world.Tick();
	const int left = world.dungeon.BossHealth();
	REQUIRE(left < full);
	REQUIRE(left > 0);
	const int minions = world.dungeon.LivingMinions();

	world.SaveAndLoad();
	world.dungeon.SpawnMonster(6, row); // his tile in view (it may be already)
	world.Tick();
	CHECK(world.dungeon.BossHealth() == left);
	CHECK(world.dungeon.LivingMinions() == minions);
}

TEST_CASE("a save from before the monsters' health loads as before") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/hitbox_rat"));
	std::ostringstream now;
	world.dungeon.Dump(now);
	const std::string text = now.str();
	const size_t tag = text.find("monsters:");
	REQUIRE(tag != std::string::npos);
	std::istringstream old(text.substr(0, tag) + "next");
	REQUIRE(world.dungeon.LoadDump(old));
	std::string rest;
	old >> rest;
	CHECK(rest == "next");
}
