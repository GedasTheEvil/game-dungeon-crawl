// Sub-cells (plan subcell-physics): the player walks, jumps and falls against the solid sub-cells of
// tests/levels/subcells; arrows and sunbeams stop at them. The drawing: tests/scenarios/solved/subcell_physics.txt.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/world/sun_beam.h"
#include <algorithm>

namespace {
// Row 2 of tests/levels/subcells, by column: 3 a quarter step, 4 a half step, 5 three quarters, 6 a stair down to the
// right, 8 a stair up to the right, 9 a half step, 10 and 11 two lips, 13-14 a ceiling three quarters up, 15-16 half
// up, 19 a quarter step in half water, 22 one in sand, 25-26 a crawl space a quarter high.
constexpr int ROW = 2;
constexpr float FLOOR = 2.f;

bool heard(const SimWorld& world, WorldSound sound) {
	return std::any_of(world.said.begin(), world.said.end(), [&](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Sound && e.sound == sound;
	});
}

void load(SimWorld& world) {
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/subcells"));
}

void placeAt(SimWorld& world, int col) {
	REQUIRE(world.dungeon.PlacePlayer(col, ROW));
	world.Tick();
}

// A jump the way the player faces, with the walk key held, until they stand again.
void jumpAndLand(SimWorld& world) {
	world.Jump();
	for (int t = 0; t < 120 && (world.player.jump.jumping || world.player.jump.falling); t++) {
		world.Step(world.facing);
		world.Tick();
	}
}

Tile shaped(std::uint16_t shape) {
	Tile t;
	t.shape = shape;
	return t;
}
} // namespace

TEST_CASE("a quarter step stops the walk; a jump lands on it, and on a half step from there") {
	SimWorld world;
	load(world);
	CHECK_FALSE(world.WalkTo(3.5f)); // the step's face at x 3
	// The hitbox's side meets it, not a wider body (nor the weapon's reach).
	const float half = world.player.HalfWidth();
	CHECK(world.X() < 3.f - half + 0.01f);
	CHECK(world.X() > 3.f - half - 0.03f);
	CHECK(world.Y() == FLOOR);

	jumpAndLand(world);
	CHECK(world.Y() == doctest::Approx(FLOOR + 0.25f));
	CHECK(world.X() > 3.f);
	CHECK(world.dungeon.heightOverFloor() == doctest::Approx(0.f));

	CHECK_FALSE(world.WalkTo(4.5f)); // the half step from the quarter: a quarter up again
	jumpAndLand(world);
	CHECK(world.Y() == doctest::Approx(FLOOR + 0.5f));
}

TEST_CASE("a jump from the floor does not get onto a half step") {
	SimWorld world;
	load(world);
	placeAt(world, 10); // the floor between the half step at col 9 and the lip at the right of col 10
	CHECK(world.Y() == FLOOR);
	CHECK_FALSE(world.WalkTo(9.5f));
	const float before = world.X();
	jumpAndLand(world);
	CHECK(world.Y() == FLOOR);
	CHECK(world.X() >= 10.f + world.player.HalfWidth() - 0.06f); // still in front of its face
	CHECK(world.X() <= before);
}

TEST_CASE("walking down a stair drops a quarter at a time, the trailing foot on the step until it is off") {
	SimWorld world;
	load(world);
	placeAt(world, 5);
	CHECK(world.Y() == doctest::Approx(FLOOR + 0.75f));
	// Just past each column's edge by the hitbox's half width: the trailing foot still on that column.
	const float half = world.player.HalfWidth();
	REQUIRE(world.WalkTo(6.25f + half - 0.03f)); // the stair's top column (6 to 6.25)
	CHECK(world.Y() == doctest::Approx(FLOOR + 0.75f));
	REQUIRE(world.WalkTo(6.5f + half - 0.03f));
	world.Wait(500);
	CHECK(world.Y() == doctest::Approx(FLOOR + 0.5f));
	REQUIRE(world.WalkTo(6.75f + half - 0.03f));
	world.Wait(500);
	CHECK(world.Y() == doctest::Approx(FLOOR + 0.25f));
	REQUIRE(world.WalkTo(7.5f));
	world.Wait(500);
	CHECK(world.Y() == FLOOR);
	CHECK_FALSE(world.player.jump.falling);
}

TEST_CASE("a fall lands on a step, even from a tick's longest drop over it") {
	SimWorld world;
	load(world);
	placeAt(world, 3);
	CHECK(world.Y() == doctest::Approx(FLOOR + 0.25f));
	// A drop of FALL_MAX_STEP from just over the step's top still finds it.
	REQUIRE(world.dungeon.floorBelow(3.5f, FLOOR + 0.25f + FALL_MAX_STEP - 0.01f).has_value());
	CHECK(*world.dungeon.floorBelow(3.5f, FLOOR + 0.25f + FALL_MAX_STEP - 0.01f) == doctest::Approx(FLOOR + 0.25f));
	CHECK(world.dungeon.floorUnder(3.5f, FLOOR + 0.9f) == doctest::Approx(FLOOR + 0.25f));
	CHECK(world.dungeon.floorUnder(2.5f, FLOOR + 0.9f) == FLOOR); // beside it, the cell's floor
	// A foot on the step carries the player; the centre over the floor beside it.
	const float half = world.player.HalfWidth();
	CHECK(world.dungeon.floorUnder(3.f - half + 0.02f, FLOOR + 0.5f) == doctest::Approx(FLOOR + 0.25f));
	CHECK(world.dungeon.floorUnder(3.f - half - 0.01f, FLOOR + 0.5f) == FLOOR);
}

TEST_CASE("a jump stops at a low ceiling with a bump, then comes back down") {
	SimWorld world;
	load(world);
	placeAt(world, 13); // a ceiling three quarters up
	CHECK(world.Y() == FLOOR);
	world.Jump();
	float top = world.Y();
	for (int t = 0; t < 100; t++) {
		world.Tick();
		top = std::max(top, world.Y());
	}
	CHECK(top <= FLOOR + 0.75f - PLAYER_BODY_HEIGHT + 0.001f);
	CHECK(top > FLOOR + 0.3f); // it did rise
	CHECK(heard(world, WorldSound::HeadBump));
	CHECK(world.Y() == FLOOR);
	CHECK_FALSE(world.player.jump.jumping);

	SimWorld open; // the same jump under the hall's own ceiling: no bump
	load(open);
	placeAt(open, 2);
	open.Jump();
	open.Wait(1500);
	CHECK_FALSE(heard(open, WorldSound::HeadBump));
}

TEST_CASE("the player walks under a ceiling half up, not into a crawl space a quarter high") {
	SimWorld world;
	load(world);
	placeAt(world, 17);
	REQUIRE(world.WalkTo(13.5f)); // under the half and the three-quarter ceilings
	CHECK(world.Y() == FLOOR);

	placeAt(world, 24);
	CHECK_FALSE(world.WalkTo(25.5f));
	CHECK(world.X() < 25.f - world.player.HalfWidth() + 0.01f);
	CHECK(world.X() > 25.f - world.player.HalfWidth() - 0.03f);
}

TEST_CASE("a step in half water and in sand stands out of them") {
	SimWorld world;
	load(world);
	placeAt(world, 18);
	world.WaitTicks(5);
	CHECK(world.dungeon.PlayerWading());
	CHECK(world.dungeon.PlayerSink() > 0.f);
	placeAt(world, 19);
	CHECK(world.Y() == doctest::Approx(FLOOR + 0.25f));
	CHECK_FALSE(world.dungeon.PlayerWading());
	CHECK(world.dungeon.PlayerSink() == 0.f);
	placeAt(world, 21);
	CHECK(world.dungeon.PlayerInSand());
	placeAt(world, 22);
	CHECK_FALSE(world.dungeon.PlayerInSand());
}

TEST_CASE("an arrow stops at a solid sub-cell") {
	SimWorld world;
	load(world);
	placeAt(world, 7); // the stair up and the half step ahead; nothing to aim at: the floor ARROW_FREE_RANGE away
	world.Equip(ItemKind::SelfBow);
	world.Wait(1000);
	world.facing = 1;
	REQUIRE(world.Attack());
	world.Wait(1500);
	const std::vector<float> stuck = world.dungeon.StuckMissileXs();
	REQUIRE(stuck.size() == 1);
	CHECK(stuck[0] > 8.f);
	CHECK(stuck[0] < 7.5f + ARROW_FREE_RANGE - 0.1f);
}

TEST_CASE("a sunbeam stops at a solid sub-cell on its line, the middle of the cell") {
	CHECK(beamReach(Tile{}, BeamDir::Right) == 1.f);
	CHECK(beamReach(shaped(0x000f), BeamDir::Right) == 1.f); // a quarter step: under the beam
	CHECK(beamReach(shaped(0x00ff), BeamDir::Left) == 1.f); // a half step: the beam just over it
	CHECK(beamReach(shaped(0x0fff), BeamDir::Right) == 0.f); // three quarters: in its face
	CHECK(beamReach(shaped(0x08ce), BeamDir::Right) == doctest::Approx(0.75f)); // the stair's top column
	CHECK(beamReach(shaped(0x08ce), BeamDir::Left) == 0.f);
	CHECK(beamReach(shaped(0x000f), BeamDir::Down) == doctest::Approx(0.75f)); // down onto the step
	CHECK(beamReach(shaped(0xf000), BeamDir::Up) == doctest::Approx(0.75f));
	CHECK_FALSE(blocksBeam(shaped(0x000f), BeamDir::Right));
	CHECK(blocksBeam(shaped(0x000f), BeamDir::Down));
}

TEST_CASE("a shaped cell's top row all solid is a floor for the cell above") {
	CHECK(isFloor(shaped(0xf000)));
	CHECK(isFloor(shaped(0xffff)));
	CHECK_FALSE(isFloor(shaped(0x7000)));
	CHECK_FALSE(isFloor(shaped(0x000f)));
	CHECK_FALSE(topRowSolid(wallTile())); // a wall is a floor as a solid cell
	CHECK(isFloor(wallTile()));
}

// Head bump (plan head-bump): the jump under the half-up ceiling (col 15) hurts, the three-quarter one (col 13) only
// grazes; Player::HeadBump is the shared damage and daze.
namespace {
int bumpJump(SimWorld& world, int col) {
	REQUIRE(world.Load("tests/levels/subcells"));
	REQUIRE(world.dungeon.PlacePlayer(col, ROW));
	world.Tick();
	const int before = world.player.stats.CurrentHP();
	world.Jump();
	world.Wait(1500);
	return before - world.player.stats.CurrentHP();
}
} // namespace

TEST_CASE("a jump into a low ceiling costs 5 HP, 15 at 300 max HP; a graze or an open hall costs nothing") {
	SimWorld world;
	CHECK(bumpJump(world, 15) == HEAD_BUMP_MIN_HP); // half up: the head hits it still rising fast
	CHECK(heard(world, WorldSound::HeadBump));
	CHECK(world.player.Alive());

	SimWorld high;
	REQUIRE(high.Load("tests/levels/subcells"));
	high.player.stats.AddMaxHP(500); // 50 -> 300, healed
	REQUIRE(high.dungeon.PlacePlayer(15, ROW));
	high.Tick();
	high.Jump();
	high.Wait(1500);
	CHECK(high.player.stats.CurrentHP() == 285);

	SimWorld graze; // three quarters up: the top of the jump only just reaches it (a knock, no hurt)
	CHECK(bumpJump(graze, 13) == 0);
	CHECK(heard(graze, WorldSound::HeadBump));

	SimWorld open;
	CHECK(bumpJump(open, 2) == 0);
}

TEST_CASE("the daze rolls about half the time, 1 to 2 s; a bump at 5 HP or less kills; a dazed player cannot jump") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/subcells"));
	WorldEvents events;
	Rng rng(7);
	int dazed = 0;
	const int rolls = 400;
	for (int i = 0; i < rolls; i++) {
		world.player.stats.HealFully();
		world.player.dazeMs = 0;
		CHECK(world.player.HeadBump(events, rng) == HEAD_BUMP_MIN_HP);
		if (world.player.Dazed()) {
			dazed++;
			CHECK(world.player.dazeMs >= HEAD_BUMP_DAZE_MIN_MS);
			CHECK(world.player.dazeMs <= HEAD_BUMP_DAZE_MAX_MS);
		}
	}
	CHECK(dazed > rolls * 40 / 100);
	CHECK(dazed < rolls * 60 / 100);

	world.player.dazeMs = 1500;
	world.Jump();
	CHECK_FALSE(world.player.jump.jumping);

	world.player.stats.HealFully();
	world.player.stats.LoseHP(world.player.stats.CurrentHP() - HEAD_BUMP_MIN_HP);
	world.player.HeadBump(events, rng);
	CHECK_FALSE(world.player.Alive());
}
