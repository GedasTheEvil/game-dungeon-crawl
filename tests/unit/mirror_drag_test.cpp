// Dragging mirrors (plan mirror-drag): the drag key takes hold of the mirror in the next cell, the player crouches
// facing it and pulls (or pushes) it a cell at a time at a quarter of the walk, only along the flat corridor; no
// attack, no jump; the key again lets go. Dragged under a light shaft and turned, the mirror lights a sun disc.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
// tests/levels/mirror_drag, row 2: the mirror at col 5 (pointing left), the light shaft over col 8 shining down, the
// red sun disc at col 14, the red gate at col 16, a second mirror at col 22, sand at col 23, half water at col 24.
constexpr int ROW = 2;

bool mirrorAt(const SimWorld& world, int col) { return world.dungeon.Cell(col, ROW).type == Mirror; }

SimWorld& loaded(SimWorld& world, int col) {
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/mirror_drag"));
	REQUIRE(world.dungeon.PlacePlayer(col, ROW));
	world.Tick();
	return world;
}
} // namespace

TEST_CASE("the mirror pulled under the light shaft and turned there lights the sun disc") {
	SimWorld world;
	loaded(world, 6);
	CHECK_FALSE(world.dungeon.Sunlit(14, ROW));
	REQUIRE(world.dungeon.ToggleDrag(-1)); // the mirror in the next cell to the left
	CHECK(world.X() == doctest::Approx(5.5f + DRAG_HOLD)); // a step up to it
	world.Tick();
	CHECK(world.dungeon.Dragging());
	CHECK(world.player.crouched);
	CHECK(world.dungeon.DragFacing() == -1);

	REQUIRE(world.WalkTo(8.75f)); // backwards, pulling it: a cell at a time, a cell behind the player
	CHECK(world.dungeon.DragFacing() == -1); // still looking at it
	CHECK_FALSE(mirrorAt(world, 5));
	CHECK(mirrorAt(world, 8));

	world.dungeon.ToggleDrag(-1); // let go
	world.Wait(300);
	CHECK_FALSE(world.dungeon.Dragging());
	CHECK_FALSE(world.player.crouched);
	CHECK(mirrorAt(world, 8));

	REQUIRE(world.WalkTo(8.5f));
	world.Interact(); // left to up
	world.Interact(); // up to right: the beam runs along the row to the disc
	world.Wait(100);
	CHECK(world.dungeon.Sunlit(14, ROW));
	CHECK(world.Told("The Aten blazes"));
}

TEST_CASE("pushed back the other way, the mirror goes ahead of the player") {
	SimWorld world;
	loaded(world, 6);
	REQUIRE(world.dungeon.ToggleDrag(-1));
	REQUIRE(world.WalkTo(3.75f));
	CHECK(mirrorAt(world, 3));
	CHECK_FALSE(mirrorAt(world, 5));
	CHECK(world.dungeon.DragFacing() == -1);
	CHECK_FALSE(world.WalkTo(1.5f)); // the mirror cannot go into the entrance's cell, so the player stops too
	CHECK(mirrorAt(world, 2));
	CHECK(world.X() >= 2.f + DRAG_HOLD);
}

TEST_CASE("dragging walks at three quarters of the crouched walk, a quarter of the walk") {
	SimWorld standing;
	loaded(standing, 7);
	const float from = standing.X();
	standing.HoldWalk(1, 1000);
	const float walk = standing.X() - from;
	REQUIRE(walk > 0.5f);

	SimWorld world;
	loaded(world, 6);
	REQUIRE(world.dungeon.ToggleDrag(-1));
	world.Tick();
	const float start = world.X();
	world.HoldWalk(1, 1000);
	CHECK(world.X() - start == doctest::Approx(walk * CROUCH_WALK_FACTOR * DRAG_WALK_FACTOR).epsilon(0.05));
}

TEST_CASE("dragging, no attack and no jump; let go, both work") {
	SimWorld world;
	loaded(world, 6);
	world.Equip(ItemKind::Club);
	world.Wait(1100); // the attack timer of the load
	REQUIRE(world.dungeon.ToggleDrag(-1));
	world.Tick();
	world.Jump();
	world.Tick();
	CHECK_FALSE(world.player.jump.jumping);
	CHECK_FALSE(world.Attack());
	world.Tick();
	CHECK(world.Told("Let go to strike"));

	world.dungeon.LetGo();
	world.Tick();
	CHECK_FALSE(world.player.crouched);
	world.Wait(1100); // SimWorld::Attack's timer check restarted the timer
	CHECK(world.Attack());
	world.Wait(1500);
	world.Jump();
	world.Tick();
	CHECK(world.player.jump.jumping);
}

TEST_CASE("only on the flat corridor: not into sand, not from sand or water, nothing out of reach") {
	SimWorld world;
	loaded(world, 21);
	REQUIRE(world.dungeon.ToggleDrag(1)); // the mirror at col 22, sand beyond it
	CHECK_FALSE(world.WalkTo(22.9f)); // pushed, it does not go into the sand: the player stays put too
	CHECK(mirrorAt(world, 22));
	CHECK_FALSE(world.WalkTo(20.5f)); // pulled, it comes along the bare floor up to the exit
	CHECK(mirrorAt(world, 21));
	world.dungeon.LetGo();

	SimWorld sand;
	loaded(sand, 23);
	CHECK_FALSE(sand.dungeon.ToggleDrag(-1)); // the player in the sand
	sand.Tick();
	CHECK(sand.Told("only on bare, flat floor"));
	CHECK_FALSE(sand.dungeon.Dragging());

	SimWorld far;
	loaded(far, 12);
	CHECK_FALSE(far.dungeon.ToggleDrag(-1)); // no mirror in the next cell
	far.Tick();
	CHECK(far.Told("Nothing here to drag"));
}

TEST_CASE("a load lets go; the dragged mirror stays where it was left") {
	SimWorld world;
	loaded(world, 6);
	REQUIRE(world.dungeon.ToggleDrag(-1));
	REQUIRE(world.WalkTo(6.75f));
	REQUIRE(mirrorAt(world, 6));
	REQUIRE(world.dungeon.PlacePlayer(12, ROW)); // out of reach at once (a teleport)
	world.Tick();
	CHECK_FALSE(world.dungeon.Dragging());
	CHECK(mirrorAt(world, 6));
}
