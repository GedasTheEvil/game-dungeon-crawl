// World rules on the sim harness (docs/plan/scenarios-to-unit-tests.md technique 7), ported from the scenarios that
// checked them with `expect` lines only: teleporters, the exit, dart traps, rock falls, the fall trap, arrows in
// water, the boss fights and their saves.
#include "../../external/doctest/doctest.h"
#include "../../src/world/level_gen.h"
#include "sim_world.h"

TEST_CASE("a teleporter jumps the player to its partner and back; the sealed room holds the exit") {
	SimWorld world; // tests/levels/teleport: row 2 -> row 4
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/teleport"));
	world.WaitTicks(30);
	REQUIRE(world.Walk(5.3f));
	world.WaitTicks(10);
	CHECK(world.Y() < 3);
	world.Interact();
	world.WaitTicks(5);
	CHECK(world.Y() > 3);
	CHECK(world.X() > 7.4f);
	CHECK(world.X() < 8.f);
	world.Interact();
	world.WaitTicks(5);
	CHECK(world.Y() < 3);
	CHECK(world.X() > 6.4f);
	CHECK(world.X() < 7.f);
	world.Interact();
	world.WaitTicks(5);
	REQUIRE(world.Walk(2.f));
	world.Interact();
	world.WaitTicks(5);
	CHECK(world.dungeon.LevelNumber() == 2);
}

TEST_CASE("leaving a level mid-jump does not carry the jump into the next level") {
	SimWorld world; // tests/levels/classic2 exits at row 36; lvl2 starts at row 9 under a one-cell ceiling
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/classic2"));
	world.WaitTicks(30);
	REQUIRE(world.Walk(3.45f));
	REQUIRE(world.Climb(-1.97f));
	REQUIRE(world.Walk(-4.f));
	REQUIRE(world.Climb(-1.97f));
	REQUIRE(world.Walk(7.f));
	REQUIRE(world.Climb(2.03f));
	REQUIRE(world.Walk(9.f));
	world.Jump();
	world.WaitTicks(3);
	REQUIRE(world.player.jump.jumping);
	world.Interact();
	world.Wait(2000);
	CHECK(world.dungeon.LevelNumber() == 2);
	CHECK(world.Y() == 9.f);
	CHECK(world.Walk(2.f));
}

TEST_CASE(
	"a dart trap's plate shoots poison darts across the corridor; a jump over it, nothing; the Anubis sets it off") {
	SimWorld world; // tests/levels/dart_trap: the plate at col 4, another at col 12 with an Anubis guard beyond
	REQUIRE(world.Load("tests/levels/dart_trap"));
	world.WaitTicks(30);
	CHECK(world.Hp() == 50);
	CHECK(world.journal.Notes().empty());
	REQUIRE(world.WalkTo(4.5f));
	world.Wait(240);
	REQUIRE(world.Walk(1.f)); // walking on does not get the player clear
	world.Wait(1000);
	CHECK(world.player.stats.poison.Mask() == 2); // medium
	CHECK(world.Hp() < 50);
	CHECK(world.Hp() >= 36); // 3 darts (6) and about 2 s of poison (3 a second)
	// The game writes the notes as it drains the events: the dart traps, the poison (and the hit points).
	int notes = 0;
	for (const WorldEvent& e : world.said)
		notes += e.kind == WorldEvent::Kind::Note ? 1 : 0;
	CHECK(notes >= 2);

	REQUIRE(world.Load("tests/levels/dart_trap"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(3.85f));
	world.facing = 1;
	world.Jump();
	REQUIRE(world.Walk(2.f));
	world.Wait(1000);
	CHECK(world.player.stats.poison.Mask() == 0);
	CHECK(world.Hp() == 50);

	world.player.god = true;
	REQUIRE(world.WalkTo(10.3f));
	world.Wait(2000);
	CHECK(world.Nearest() == 600);
	world.Wait(16000);			  // slow: it takes this long to get onto the plate
	CHECK(world.Nearest() < 600); // a quarter of the darts' damage (trapDamagePct 25); immune to the poison
	CHECK(world.Nearest() > 590);
}

TEST_CASE("a rock fall: walk on and clear it, stop at its edge and get grazed, stand in its middle and be crushed") {
	SimWorld world; // tests/levels/rock_fall: rocks at cols 5, 10, 15, 20, 25; a level 2 player (62 HP)
	REQUIRE(world.Load("tests/levels/rock_fall"));
	world.WaitTicks(30);
	world.player.stats.AddXP(1000, world.events);
	CHECK(world.Hp() == 62);
	REQUIRE(world.Walk(3.9f)); // col 5: walking on without stopping gets the player clear
	REQUIRE(world.Walk(3.f));
	world.Wait(1200);
	CHECK(world.Hp() == 62);
	REQUIRE(world.Walk(3.f)); // col 10: stopping at the far edge of the cell, the rock clips a leg
	world.Wait(1200);
	CHECK(world.Hp() == 12);
	REQUIRE(world.Walk(4.3f)); // col 15: stepping in and back gets the player clear
	REQUIRE(world.Walk(-1.f));
	world.Wait(1200);
	CHECK(world.Hp() == 12);
	REQUIRE(world.Walk(11.3f)); // col 25, through col 20: in the middle of the cell, the rock crushes the player
	world.Wait(1250);
	CHECK_FALSE(world.player.Alive());
}

TEST_CASE("walking into a spike pit: the fall is fast, the ramping spikes kill within two seconds") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/fall_trap"));
	world.WaitTicks(30);
	world.Walk(2.5f); // it falls on the way
	world.Wait(1000);
	CHECK(world.Y() == 36.f);
	CHECK(world.Hp() > 40);
	world.Wait(2000);
	CHECK_FALSE(world.player.Alive());
}

TEST_CASE("an arrow into a monster in half water hits for half, on dry floor in full") {
	SimWorld world; // tests/levels/water_arrow: a giant scarab (90 HP, weak to pierce) in a pool at cols 4-7
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/water_arrow"));
	world.Equip(ItemKind::SelfBow);
	REQUIRE(world.WalkTo(2.8f));
	world.HoldWalk(1, 2 * UPDATE_TICK_MS);
	world.WaitTicks(10);
	CHECK(world.Nearest() == 90);
	world.Attack();
	world.Wait(2000);
	CHECK(world.Nearest() == 78);
	REQUIRE(world.WalkTo(1.5f));
	world.HoldWalk(1, 2 * UPDATE_TICK_MS);
	world.Wait(30000); // out of the water (its centre past col 4), still three tiles off
	world.Attack();
	world.Wait(1000);
	CHECK(world.Nearest() == 54);
}

TEST_CASE("the boss fight: summons, the sealed gate, its chest, and it stays dead after a save and load") {
	SimWorld world; // tests/levels/boss: the teleporter to the sealed boss room
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/boss"));
	world.WaitTicks(30);
	REQUIRE(world.Walk(3.5f));
	world.Interact();
	world.WaitTicks(5);
	CHECK(world.Y() > 3);
	REQUIRE(world.Walk(5.f));
	world.WaitTicks(5);
	CHECK(world.dungeon.BossHealth() == 320);
	CHECK(world.dungeon.LivingMinions() >= 3);
	world.Wait(5000);
	CHECK(world.dungeon.LivingMinions() == 5); // at most 5 alive

	world.Equip(ItemKind::ShortSword);
	world.WaitTicks(2);
	for (int swing = 0; swing < 6; swing++) {
		world.Attack();
		world.Wait(1000);
	}
	CHECK(world.Xp() < 10); // a minion gives 1 XP while the boss lives
	world.Walk(4.4f);		// the boss gate is sealed while it lives
	world.Wait(1000);
	CHECK(world.X() < 12.f);

	CHECK(world.dungeon.ChestCount() == 2);
	world.dungeon.HurtBoss(world.dungeon.BossHealth());
	world.WaitTicks(5);
	CHECK(world.dungeon.BossHealth() == 0);
	CHECK(world.Xp() >= 6000);
	world.Wait(4000);
	CHECK(world.dungeon.ChestCount() == 3); // a copy of a weapon the player has, once its body has died away

	world.SaveAndLoad();
	world.WaitTicks(5);
	CHECK(world.dungeon.BossHealth() == 0);
	CHECK(world.dungeon.LivingMinions() == 0);
}

TEST_CASE("level 5 from its entrance to its exit: the boss room, the blue key, the blue gate") {
	SimWorld world; // the path levelcheck --script found
	world.player.god = true;
	REQUIRE(world.Load(5));
	world.WaitTicks(30);
	REQUIRE(world.Walk(25.45f));
	REQUIRE(world.Climb(-1.97f));
	REQUIRE(world.Walk(4.f));
	world.Interact();
	world.WaitTicks(5);
	REQUIRE(world.Walk(7.f));
	world.Wait(3000);
	CHECK(world.dungeon.LivingMinions() >= 3);
	REQUIRE(world.Walk(2.95f));
	world.Wait(500);
	world.dungeon.HurtBoss(world.dungeon.BossHealth());
	world.Wait(1000);
	REQUIRE(world.Walk(4.15f));
	world.Wait(1400);
	REQUIRE(world.Walk(2.85f));
	CHECK(world.dungeon.KeysHeld() == 2); // blue
	REQUIRE(world.Walk(-1.05f));
	world.Wait(1400);
	REQUIRE(world.Walk(-15.95f));
	world.Interact();
	world.WaitTicks(5);
	REQUIRE(world.Walk(-17.1f));
	world.Wait(1400);
	REQUIRE(world.Walk(-9.95f));
	world.Interact();
	world.WaitTicks(5);
	CHECK(world.dungeon.LevelNumber() == 6);
}

TEST_CASE(
	"the Anubis boss: mummies climb out of his coffins, his blow takes 129 HP of a level 55 player, the gold key") {
	SimWorld world; // tests/levels/anubis_boss: six coffins round the boss
	REQUIRE(world.Load("tests/levels/anubis_boss"));
	world.player.stats.AddXP(1800200, world.events);
	world.WaitTicks(30);
	CHECK(world.Hp() == 698);
	REQUIRE(world.Walk(3.5f));
	world.Interact();
	world.WaitTicks(5);
	CHECK(world.Y() > 3);
	REQUIRE(world.WalkTo(9.f));
	world.WaitTicks(10);
	CHECK(world.dungeon.BossHealth() == 2400);
	CHECK(world.dungeon.LivingMinions() >= 2);
	world.WaitTicks(20);
	world.Wait(6000);
	CHECK(world.dungeon.LivingMinions() >= 3);
	world.Wait(1000); // he has walked up and struck by now
	CHECK(world.Hp() <= 569);
	CHECK(world.player.Alive());

	world.player.god = true;
	world.dungeon.HurtBoss(world.dungeon.BossHealth());
	world.WaitTicks(5);
	CHECK(world.dungeon.BossHealth() == 0);
	world.Wait(2000);
	REQUIRE(world.WalkTo(23.5f));
	world.Wait(1000);
	CHECK(world.dungeon.KeysHeld() == 8); // gold
}

TEST_CASE("the Anubis boss's coffins stand round his tile, also in a game saved after his death") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/anubis_boss"));
	world.player.stats.AddXP(112000, world.events);
	world.player.god = true;
	world.WaitTicks(30);
	CHECK(world.dungeon.CoffinCount() == 6);
	REQUIRE(world.Walk(3.5f));
	world.Interact();
	world.WaitTicks(5);
	REQUIRE(world.WalkTo(9.f));
	world.WaitTicks(10);
	CHECK(world.dungeon.BossHealth() == 2400);
	world.dungeon.HurtBoss(world.dungeon.BossHealth());
	world.WaitTicks(5);
	CHECK(world.dungeon.BossHealth() == 0);
	world.SaveAndLoad();
	world.WaitTicks(5);
	CHECK(world.dungeon.BossHealth() == 0);
	CHECK(world.dungeon.CoffinCount() == 6);
}

TEST_CASE("a generated level and the campaign levels load and play") {
	SimWorld world;
	GenOptions options;
	options.seed = 3;
	options.difficulty = 4;
	const GenResult gen = generateLevel(options);
	REQUIRE(gen.ok);
	world.random.Seed(3);
	world.dungeon.LoadGrid(gen.grid, "gen:3:4", genDecorDepth(4));
	world.player.Reanimate();
	world.WaitTicks(30);
	CHECK(world.player.Alive());
	for (const int level : {6, 15}) {
		REQUIRE(world.Load(level, 3));
		world.WaitTicks(30);
		CHECK(world.dungeon.LevelNumber() == level);
	}
}

TEST_CASE("a chest holds its main item and bonus rolls on the gameplay stream") {
	SimWorld world; // tests/levels/classic1: the chest at x 14 on the start row holds a sword
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/classic1", 4));
	REQUIRE(world.Walk(3.f));
	REQUIRE(world.Walk(1.6f));
	world.facing = 1;
	world.Jump();
	REQUIRE(world.Walk(2.f));
	REQUIRE(world.Walk(0.6f));
	world.Interact();
	CHECK(world.items.Count(ItemKind::ShortSword) == 1);
	int smallStamina = 0; // seed 4 rolls a small stamina bonus (potion 5)
	for (int k = 0; k < ITEM_KIND_COUNT; k++)
		if (isPotion(itemAt(k)) && fileIdOf(itemAt(k)).id == 5)
			smallStamina = world.items.Count(itemAt(k));
	CHECK(smallStamina == 1);
}
