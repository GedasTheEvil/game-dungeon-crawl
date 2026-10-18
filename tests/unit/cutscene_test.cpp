// Cut-scenes (plan cinematic-effects): a lever or a boss's death that opens gates off the screen shows them, the world
// standing still meanwhile; a skip ends it at once; with the option off, nothing changes.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
constexpr int ROW = 1; // tests/levels/cutscene: the corridor's row
constexpr int UPPER_ROW = 8;

GateState gateAt(const SimWorld& world, int col, int row) { return gateState(world.dungeon.Cell(col, row)); }

// tests/levels/cutscene, the player by the lever, the rat behind the near gate.
void atLever(SimWorld& world, bool cutScenes) {
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/cutscene"));
	world.dungeon.SetCutScenes(cutScenes);
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(6.5f));
	world.WaitTicks(5);
}
} // namespace

TEST_CASE("a lever's far gates: the camera goes to the nearer, then the other, and back; the world stands still") {
	SimWorld world;
	atLever(world, true);
	const Monster* rat = world.dungeon.NearestMonster();
	REQUIRE(rat != nullptr);
	world.Interact();
	REQUIRE(world.dungeon.CutScenePlaying());
	const float ratX = rat->CentreX();
	const float playerX = world.X();

	world.Wait(CUTSCENE_PAN_MS + CUTSCENE_HOLD_MS / 2); // on the gate along the row
	CutShot shot = world.dungeon.CutSceneShot();
	CHECK(shot.dx == doctest::Approx(19.5f - playerX));
	CHECK(shot.dy == doctest::Approx(0.f));
	CHECK(shot.look == doctest::Approx(1.f));
	CHECK(gateAt(world, 19, ROW) == GateState::Opening);
	CHECK(gateAt(world, 19, UPPER_ROW) == GateState::Opening); // its motion waits for the camera
	CHECK(gateAt(world, 8, ROW) == GateState::Open); // on the screen: it opened at once

	world.Wait(CUTSCENE_PAN_MS + CUTSCENE_HOLD_MS); // on the upper gate
	shot = world.dungeon.CutSceneShot();
	CHECK(shot.dx == doctest::Approx(19.5f - playerX));
	CHECK(shot.dy == doctest::Approx(static_cast<float>(UPPER_ROW) - world.Y()));
	CHECK(gateAt(world, 19, ROW) == GateState::Open);

	CHECK(rat->CentreX() == ratX); // the gate before it is open, but the world stands still
	CHECK(world.X() == playerX);

	world.Wait(CUTSCENE_HOLD_MS / 2 + CUTSCENE_PAN_MS + 50); // back to the player
	CHECK_FALSE(world.dungeon.CutScenePlaying());
	shot = world.dungeon.CutSceneShot();
	CHECK(shot.dx == 0.f);
	CHECK(shot.look == 0.f);
	CHECK(gateAt(world, 19, UPPER_ROW) == GateState::Open);
	world.WaitTicks(30);
	CHECK(rat->CentreX() != ratX); // the world goes on
}

TEST_CASE("a skipped cut-scene: back at once, the far gates open without waiting for the camera") {
	SimWorld world;
	atLever(world, true);
	world.Interact();
	world.Wait(300);
	REQUIRE(world.dungeon.CutScenePlaying());
	world.dungeon.SkipCutScene();
	CHECK_FALSE(world.dungeon.CutScenePlaying());
	CHECK(world.dungeon.CutSceneShot().dx == 0.f);
	world.Wait(GATE_OPEN_MS + 50);
	CHECK(gateAt(world, 19, ROW) == GateState::Open);
	CHECK(gateAt(world, 19, UPPER_ROW) == GateState::Open);
}

TEST_CASE("cut-scenes off: the lever opens its gates unseen, the world goes on") {
	SimWorld world;
	atLever(world, false);
	world.Interact();
	world.WaitTicks(1);
	CHECK_FALSE(world.dungeon.CutScenePlaying());
	world.Wait(GATE_OPEN_MS + 50);
	CHECK(gateAt(world, 19, UPPER_ROW) == GateState::Open);
}

TEST_CASE("a gate on the screen shows no cut-scene; a boss's death shows its far boss gate") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/mechanisms")); // its blue gate is two cells from the lever
	world.dungeon.SetCutScenes(true);
	world.WaitTicks(30);
	REQUIRE(world.Walk(4.1f)); // the red key, its gate
	REQUIRE(world.Walk(2.55f));
	world.Wait(GATE_OPEN_MS);
	REQUIRE(world.Walk(2.45f));
	world.Interact();
	world.WaitTicks(1);
	CHECK_FALSE(world.dungeon.CutScenePlaying());

	REQUIRE(world.Load("tests/levels/cutscene_boss"));
	world.dungeon.SetCutScenes(true);
	world.WaitTicks(30);
	REQUIRE(world.dungeon.BossHealth() > 0);
	world.dungeon.HurtBoss(world.dungeon.BossHealth());
	world.WaitTicks(2);
	REQUIRE(world.dungeon.CutScenePlaying());
	world.Wait(CUTSCENE_PAN_MS + CUTSCENE_HOLD_MS / 2);
	CHECK(world.dungeon.CutSceneShot().dx == doctest::Approx(19.5f - world.X()));
}
