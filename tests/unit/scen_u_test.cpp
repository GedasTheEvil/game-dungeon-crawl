#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/world/loot.h"
#include <map>

// Former scenarios (attack_recovery, ladder_jump_off, loot, monster_slots, spikes, stamina, trap_hop, generated_path).

TEST_CASE("moving or sprinting does not cancel an attack recovery: the club swings again 900 ms after the first") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/archery"));
	world.Equip(ItemKind::Club);
	world.WaitTicks(80);
	REQUIRE(world.Attack());
	world.Tick();
	CHECK(world.player.attackStartMs >= 0);
	world.player.stats.SetSprintRequested(true);
	world.HoldWalk(1, 700);
	CHECK(world.player.attackStartMs < 0); // the swing is over, the recovery is not
	CHECK_FALSE(world.Attack());
	world.HoldWalk(1, 250);
	world.player.stats.SetSprintRequested(false);
	CHECK(world.Attack());
}

TEST_CASE("a jump off the ladder lands on the ledge, not in the floor below") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/classic1"));
	world.WaitTicks(30);
	REQUIRE(world.Walk(3.f));
	REQUIRE(world.Walk(1.6f));
	world.facing = 1;
	world.Jump();
	REQUIRE(world.Walk(2.f));
	world.Wait(1000);
	REQUIRE(world.Walk(3.4f));
	world.WaitTicks(10);
	REQUIRE(world.Climb(-3.f));
	world.HoldWalk(1, 400); // to the wall
	world.Jump();
	world.Wait(2000);
	CHECK(world.Y() == doctest::Approx(39.f));
	CHECK(world.Walk(1.f));
	CHECK(world.X() > 19.5f);
	CHECK(world.player.Alive());
}

TEST_CASE("chest bonus loot: stamina potions, a lower weapon the player holds, small health in a large health chest") {
	Rng rng(1);
	OwnedWeapons owned{};
	owned[static_cast<size_t>(itemIndex(ItemKind::Spear))] = true;
	owned[static_cast<size_t>(itemIndex(ItemKind::Club))] = true;
	std::map<ItemKind, int> got;
	for (int i = 0; i < 1000; i++)
		for (ItemKind k : RollChestLoot(ItemKind::ShortSword, owned, 1, rng))
			got[k]++;
	CHECK(got[ItemKind::ShortSword] == 1000);
	CHECK(got[ItemKind::SmallStamina] > 250); // 30%
	CHECK(got[ItemKind::SmallStamina] < 350);
	CHECK(got[ItemKind::LargeStamina] > 150); // 20%
	CHECK(got[ItemKind::LargeStamina] < 250);
	CHECK(got[ItemKind::Spear] > 6); // 5% lower weapon, one held
	CHECK(got[ItemKind::Club] > 6);
	got.clear();
	for (int i = 0; i < 1000; i++)
		for (ItemKind k : RollChestLoot(ItemKind::LargeHealth, owned, 1, rng))
			got[k]++;
	CHECK(got[ItemKind::LargeHealth] >= 1000 + 25); // 5% the same potion
	CHECK(got[ItemKind::SmallHealth] > 70); // 10%
	CHECK(got[ItemKind::SmallHealth] < 130);
}

TEST_CASE("loading a level or a save drops the monsters of the level before") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	REQUIRE(world.Load("tests/levels/rats"));
	world.WaitTicks(30);
	CHECK(world.Hp() == 50);
	REQUIRE(world.Load("tests/levels/empty_row"));
	world.Wait(15000);
	CHECK(world.Hp() == 50);
	CHECK(world.Hp() == 50);
}

TEST_CASE(
	"spikes: standing in them, a hit every 100 ms growing by 1 every 3 hits; reset after 200 ms out, two tiles as "
	"one") {
	SimWorld world;
	world.player.stats.AddMaxHP(3900); // 2000 HP: the crossing (SPIKES_CROSS_HP, 20%: 400) does not kill
	REQUIRE(world.Load("tests/levels/spikes_one"));
	world.WaitTicks(5);
	REQUIRE(world.Walk(3.f)); // the tile edge is outside the hitbox
	world.Wait(1000);
	CHECK(world.Hp() == 2000);
	REQUIRE(world.Walk(0.5f)); // to the middle: half the crossing, and the first hits
	const int inMiddle = world.Hp();
	CHECK(2000 - inMiddle > 195);
	CHECK(2000 - inMiddle < 245); // a step past the middle
	world.Wait(1000); // standing: only the hits
	CHECK(inMiddle - world.Hp() == 30);

	REQUIRE(world.Load("tests/levels/spikes_two"));
	world.WaitTicks(5);
	REQUIRE(world.Walk(3.5f));
	world.Wait(500);
	REQUIRE(world.Walk(1.f));
	world.Wait(500);
	CHECK(world.X() > 5.4f);
	CHECK(2000 - world.Hp() > 600); // one and a half crossings
	CHECK(2000 - world.Hp() < 660); // and the hits, one streak over both
}

TEST_CASE("a jump costs stamina, it regenerates 5% a second, a level up refills it") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/classic1"));
	world.WaitTicks(30);
	CHECK(world.player.stats.Stamina() == 100);
	for (int i = 0; i < 3; i++) {
		world.Jump();
		world.Wait(1000);
	}
	CHECK(world.player.stats.Stamina() <= 60);
	world.Wait(2000);
	CHECK(world.player.stats.Stamina() >= 55);
	world.Jump();
	world.WaitTicks(5);
	CHECK(world.player.stats.Stamina() < 60);
	world.player.stats.AddXP(1000, world.events);
	CHECK(world.player.stats.Stamina() == 110);
}

TEST_CASE("a trap on the floor is jumped over from the middle of the cell before: spikes cleanly, a death trap barely "
		  "grazes") {
	SimWorld world;
	for (const char* level : {"tests/levels/spikes_one", "tests/levels/death_one"}) {
		REQUIRE(world.Load(level));
		world.WaitTicks(5);
		REQUIRE(world.WalkTo(3.45f));
		world.Jump();
		REQUIRE(world.WalkTo(5.45f));
		world.Wait(500);
		CHECK(world.X() >= 5.f);
		CHECK(world.Hp() >= (level[13] == 's' ? 50 : 40)); // the graze also poisons (death-spikes-poison)
	}
}

TEST_CASE("a generated level (seed 81, difficulty 8) plays from start to exit: key, lever, gates, jump, drops") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/gen_d8"));
	world.WaitTicks(30);
	auto right = [&](float t) { REQUIRE(world.Walk(t)); };
	auto left = [&](float t) { REQUIRE(world.Walk(-t)); };
	auto down = [&](float t) { REQUIRE(world.Climb(-t)); };
	auto up = [&](float t) { REQUIRE(world.Climb(t)); };
	right(4.45f);
	down(2.97f);
	right(1.f);
	world.Wait(1500);
	right(4.f);
	world.Jump();
	right(2.f);
	world.Wait(500);
	right(1.f);
	down(2.97f);
	right(1.f);
	world.Wait(1500);
	right(4.f);
	down(1.97f);
	left(4.f);
	right(4.f);
	up(2.03f);
	right(1.15f);
	world.Wait(1400);
	right(2.85f);
	down(3.97f);
	left(2.f);
	down(1.97f);
	left(5.f);
	world.Interact();
	world.Wait(1400);
	right(5.f);
	up(2.03f);
	left(3.05f);
	world.Wait(1400);
	left(3.95f);
	world.Wait(1500);
	left(4.f);
	down(3.97f);
	right(6.f);
	world.Wait(1500);
	right(5.f);
	down(2.97f);
	left(1.f);
	world.Wait(1500);
	left(7.f);
	world.Interact();
	world.WaitTicks(SimWorld::GATE_TRAVEL_TICKS);
	CHECK(world.dungeon.LevelNumber() == 2);
}
