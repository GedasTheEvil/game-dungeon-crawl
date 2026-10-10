#include "../../external/doctest/doctest.h"
#include "sim_world.h"

// Monster, water and mechanism checks that were scenarios (apep, cobra, crocodile, mimic, mummy, scorpion_queen,
// vampire, water, mechanisms), now on the sim.

namespace {
// The attack key, then wait gapMs: n swings.
void swing(SimWorld& world, int n, int gapMs = 640) {
	for (int i = 0; i < n; i++) {
		world.Attack();
		world.Wait(gapMs);
	}
}
} // namespace

TEST_CASE("the cobra lies coiled with no health bar, rears up, shows its bar and dies to the club") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/cobra"));
	REQUIRE(world.Walk(3.f));
	CHECK(world.dungeon.MonsterBarsShown() == 0);
	REQUIRE(world.Walk(2.f));
	world.WaitTicks(19);
	CHECK(world.dungeon.MonsterBarsShown() == 1);
	world.Wait(4000);
	CHECK(world.player.stats.poison.Mask() == 2);
	world.player.god = true;
	swing(world, 8, 1000);
	CHECK(world.Nearest() == 0);
}

TEST_CASE("the crocodile lies in the pool unseen, wakes when waded close, follows onto land and dies to the sword") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/crocodile"));
	world.Give(ItemKind::ShortSword);
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(6.8f));
	world.WaitTicks(10);
	CHECK(world.dungeon.MonsterBarsShown() == 0);
	REQUIRE(world.WalkTo(7.3f));
	world.WaitTicks(10);
	CHECK(world.dungeon.MonsterBarsShown() == 1);
	CHECK(world.Nearest() == 110);
	REQUIRE(world.WalkTo(4.2f));
	world.Wait(5000);
	REQUIRE(world.WalkTo(2.5f));
	world.Wait(3000);
	const Monster* croc = world.dungeon.NearestMonster();
	REQUIRE(croc != nullptr);
	CHECK(croc->CentreX() < 4.f); // out of the pool (cols 6-13), after the player
	world.HoldWalk(1, 32);
	world.Equip(ItemKind::ShortSword);
	world.WaitTicks(10);
	swing(world, 20, 640);
	world.Wait(3000);
	CHECK(world.Nearest() == 0);
	CHECK(world.dungeon.MonsterBarsShown() == 0);
}

TEST_CASE("a mimic looks like a chest with no bar, wakes near, dies and leaves a real chest with its loot") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/mimic", 15));
	world.WaitTicks(30);
	REQUIRE(world.Walk(2.5f));
	world.WaitTicks(10);
	CHECK(world.dungeon.MonsterBarsShown() == 0);
	REQUIRE(world.Walk(3.2f));
	world.WaitTicks(10);
	CHECK(world.dungeon.MonsterBarsShown() == 0);
	REQUIRE(world.Walk(1.3f));
	world.WaitTicks(5);
	CHECK(world.dungeon.MonsterBarsShown() == 1);
	REQUIRE(world.Walk(0.5f));
	world.Wait(2000);
	swing(world, 12, 640);
	world.Wait(3000);
	CHECK(world.dungeon.MonsterBarsShown() == 0);
	world.Interact();
	world.WaitTicks(5);
	CHECK(world.items.Count(ItemKind::LargeStamina) >= 1);
}

TEST_CASE("a mummy in its coffin shows no bar, wakes near, and leaves its coffin empty when killed") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/mummy"));
	world.player.stats.AddXP(200000, world.events);
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(8.8f));
	world.WaitTicks(10);
	CHECK(world.dungeon.MonsterBarsShown() == 0);
	REQUIRE(world.WalkTo(9.1f));
	world.WaitTicks(2);
	CHECK(world.dungeon.MonsterBarsShown() == 1);
	world.Wait(7000);
	const int coffins = world.dungeon.CoffinCount();
	swing(world, 40, 640);
	world.Wait(3000);
	CHECK(world.Nearest() == 0);
	CHECK(world.dungeon.CoffinCount() == coffins);
}

TEST_CASE("the scorpion queen's egg clusters hatch scorpions; her sting poisons strongly") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/scorpion_queen"));
	world.Give(ItemKind::ShortSword);
	REQUIRE(world.WalkTo(8.5f));
	world.Wait(2000);
	CHECK(world.dungeon.BossHealth() > 0);
	CHECK(world.dungeon.LivingMinions() >= 1);
	REQUIRE(world.WalkTo(7.5f));
	world.Equip(ItemKind::ShortSword);
	swing(world, 5, 1000);
	world.Wait(9000);
	CHECK(world.player.stats.poison.Mask() >= 4);
}

TEST_CASE("the vampire bat's roost: two bats drop, wounded it heals on bites, summons more; its death yields the key") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/vampire"));
	world.player.stats.AddXP(70000, world.events);
	world.WaitTicks(30);
	REQUIRE(world.Walk(3.5f));
	world.Interact();
	world.WaitTicks(5);
	CHECK(world.Y() > 3);
	world.WaitTicks(10);
	CHECK(world.dungeon.BossHealth() == 400);
	CHECK(world.dungeon.LivingMinions() == 2);
	world.dungeon.HurtBoss(100);
	CHECK(world.dungeon.BossHealth() == 300);
	world.Walk(5.f);
	world.Wait(4000);
	CHECK(world.Hp() < 326);
	CHECK(world.dungeon.BossHealth() > 300); // drank the blood
	world.player.god = true;
	world.Wait(4000);
	CHECK(world.dungeon.LivingMinions() >= 3);
	world.dungeon.HurtBoss(world.dungeon.BossHealth());
	world.WaitTicks(5);
	CHECK(world.dungeon.BossHealth() == 0);
	world.Wait(2000);
	world.Walk(11.f);
	world.Wait(1000);
	CHECK(world.dungeon.KeysHeld() == 8);
}

TEST_CASE("in half water: no sprint, no jump, a ladder leads out") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/water"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(6.f));
	world.HoldWalk(1, 1000);
	world.player.stats.SetSprintRequested(true);
	world.HoldWalk(1, 1000);
	world.player.stats.SetSprintRequested(false);
	CHECK(world.X() == doctest::Approx(7.f).epsilon(0.02));
	CHECK(world.player.stats.Stamina() == world.player.stats.MaxStamina());
	world.Jump();
	world.Wait(500);
	CHECK(world.Y() == 2.f);
	CHECK(world.player.stats.Stamina() == world.player.stats.MaxStamina());
	REQUIRE(world.WalkTo(9.6f));
	REQUIRE(world.Climb(2.f));
	CHECK(world.Y() >= 4.f);
}

TEST_CASE("keys, gates, lever: the key is picked up and kept through a save, the gate opens for it, the lever opens "
		  "the other") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/mechanisms"));
	world.WaitTicks(30);
	REQUIRE(world.Walk(4.1f));
	CHECK(world.dungeon.KeysHeld() == 1);
	world.SaveAndLoad();
	CHECK(world.dungeon.KeysHeld() == 1);
	REQUIRE(world.Walk(2.55f));
	world.WaitTicks(20);
	CHECK(world.X() < 8.f); // blocked until the gate is up
	world.Wait(1200);
	REQUIRE(world.Walk(2.45f));
	CHECK(world.X() > 10.f);
	world.Interact();
	world.WaitTicks(10);
	world.Wait(1300);
	REQUIRE(world.Walk(3.f));
	CHECK(world.X() > 13.f);
}
