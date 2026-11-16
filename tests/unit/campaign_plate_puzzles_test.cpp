// The campaign's pressure plate puzzles (plan campaign-plate-puzzles), played as the player would: the behaviour
// checks of its scenarios (tests/scenarios/solved/plate_*.txt).
#include "../../external/doctest/doctest.h"
#include "../../src/level/tile_defs.h"
#include "sim_world.h"
#include "../../src/sim/dungeon_cheats.h"

namespace {
SimWorld& at(SimWorld& world, int level, int col, int row) {
	world.player.god = true;
	REQUIRE(world.Load(level));
	REQUIRE(world.dungeon.PlacePlayer(col, row));
	world.WaitTicks(10);
	return world;
}

// From standing at `from`, facing `dir`: take hold, walk to `to`, let go.
void drag(SimWorld& world, float from, int dir, float to) {
	REQUIRE(world.WalkTo(from));
	world.HoldWalk(dir, 1);
	REQUIRE(world.dungeon.ToggleDrag(dir));
	world.Tick();
	REQUIRE(world.WalkTo(to));
	world.dungeon.LetGo();
}
} // namespace

TEST_CASE("lvl8: the player alone is too light for the red plate; the stone pushed onto it opens the gate") {
	SimWorld world;
	at(world, 8, 26, 26);
	world.WalkTo(27.5f);
	world.WaitTicks(60);
	CHECK_FALSE(world.WalkTo(30.6f)); // the gate at col 30 stays down
	CHECK(world.X() < 30.f);
	drag(world, 23.5f, 1, 27.05f);
	world.WaitTicks(150);
	CHECK(world.WalkTo(30.6f));
}

TEST_CASE("lvl12: off the blue plate the gate comes down before the player reaches it; the stone pulled on holds it") {
	SimWorld world;
	at(world, 12, 26, 4);
	REQUIRE(world.WalkTo(27.5f));
	world.WaitTicks(90);
	world.HoldWalk(1, 6000);
	CHECK(world.X() < 33.f);
	drag(world, 30.7f, 1, 27.25f);
	world.WaitTicks(90);
	CHECK(world.WalkTo(34.3f));
}

TEST_CASE("lvl15: the mummy follows the player onto the blue plate and holds the gate; the drop leads back down") {
	SimWorld world;
	at(world, 15, 27, 41);
	REQUIRE(world.WalkTo(22.4f));
	world.HoldWalk(1, 1);
	world.Wait(6000);
	CHECK(world.WalkTo(20.4f));
	REQUIRE(world.WalkTo(12.5f));
	world.Wait(2000);
	CHECK(world.Y() < 39.f);
}

TEST_CASE("lvl18: the High Priest's body dragged onto the blue plate opens the alcove by the boss gate") {
	SimWorld world;
	at(world, 18, 31, 2);
	world.WaitTicks(20);
	REQUIRE(world.dungeon.BossHealth() > 0);
	DungeonCheats(world.dungeon).HurtBoss(world.dungeon.BossHealth());
	world.Wait(1000);
	drag(world, 29.3f, -1, 25.75f);
	world.Wait(1500);
	CHECK(world.WalkTo(18.6f));
}

TEST_CASE("lvl24: the fallen rock on one green plate, the emptied chest on the other: the gate rises") {
	SimWorld world;
	at(world, 24, 28, 6);
	REQUIRE(world.WalkTo(30.2f));
	world.Wait(2000);
	REQUIRE(world.dungeon.Cell(29, 6).type == Stone);
	drag(world, 28.6f, 1, 31.25f);
	world.WaitTicks(90);
	REQUIRE(world.WalkTo(34.5f));
	world.WaitTicks(90);
	CHECK_FALSE(world.WalkTo(38.3f)); // one plate held: the gate at col 37 stays down
	REQUIRE(world.WalkTo(36.5f));
	world.Interact();
	world.Wait(1000);
	world.Interact();
	world.Wait(1000);
	REQUIRE(world.dungeon.ContainerEmptied(36, 6));
	REQUIRE(world.dungeon.ToggleDrag(1));
	world.Tick();
	REQUIRE(world.WalkTo(34.25f));
	world.dungeon.LetGo();
	world.WaitTicks(90);
	CHECK(world.WalkTo(38.3f));
}

TEST_CASE("lvl39: the mirror dragged under the light shaft and turned lights the red disc; the gate opens") {
	SimWorld world;
	at(world, 39, 25, 18);
	world.dungeon.SetPeace(true);
	world.HoldWalk(-1, 1);
	CHECK_FALSE(world.WalkTo(28.4f)); // the red gate at col 27
	drag(world, 23.5f, 1, 26.25f);
	world.WaitTicks(30);
	world.Interact();
	world.WaitTicks(90);
	CHECK(world.WalkTo(28.4f));
}
