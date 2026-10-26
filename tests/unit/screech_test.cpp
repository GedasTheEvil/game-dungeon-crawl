// The giant bat's screech (plan giant-bat-screech): a cone of sound along its way, walls block it; the dazed player
// walks at half speed, cannot sprint, jump or hold a ladder. The Bat of Shezmu's is wider and farther, and his bite
// heals him by all its damage while the player is dazed.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
// The level loaded, its giant bat spawned on the ceiling (col 6, the player's row) and placed by one tick, the player
// out of its sight.
const Monster* roostingBat(SimWorld& world, const char* level) {
	REQUIRE(world.Load(level));
	const auto row = static_cast<int>(std::floor(world.Y()));
	REQUIRE(world.dungeon.SpawnMonster(6, row));
	world.Tick();
	const Monster* bat = world.dungeon.NearestMonster();
	REQUIRE(bat != nullptr);
	REQUIRE(bat->Type()->id == MonsterGiantBat);
	REQUIRE(bat->Roosting());
	REQUIRE(bat->FlightDir() == 1);
	return bat;
}
} // namespace

TEST_CASE("the giant bat screeches; Shezmu's screech is wider, farther, longer, sooner, and he drinks the dazed") {
	const MonsterKind& giant = *monsterKind(MonsterGiantBat);
	const MonsterKind& shezmu = *monsterKind(MonsterVampireBat);
	REQUIRE(giant.screech.has_value());
	REQUIRE(shezmu.screech.has_value());
	CHECK_FALSE(monsterKind(MonsterBat)->screech.has_value());
	CHECK(shezmu.kin == MonsterBat);
	CHECK(giant.screech->range == doctest::Approx(4.f));
	CHECK(giant.screech->coneDegrees == doctest::Approx(60.f));
	CHECK(giant.screech->tellMs == 600);
	CHECK(giant.screech->dazeMs == 1500);
	CHECK(giant.screech->cooldownMs == 6000);
	CHECK(giant.screech->dazedHealPercent == 0);
	CHECK(shezmu.screech->range > giant.screech->range);
	CHECK(shezmu.screech->coneDegrees > giant.screech->coneDegrees);
	CHECK(shezmu.screech->dazeMs > giant.screech->dazeMs);
	CHECK(shezmu.screech->cooldownMs < giant.screech->cooldownMs);
	CHECK(shezmu.screech->dazedHealPercent == 100);
	CHECK(shezmu.screech->dazedHealPercent > shezmu.boss.lifeStealPct);
}

TEST_CASE("the cone: ahead along the bat's way, in range, not below or behind it, not through a wall") {
	SimWorld world;
	const Monster* bat = roostingBat(world, "tests/levels/giant_bat");
	const ScreechRules& rules = *bat->Type()->screech;
	const auto row = static_cast<int>(std::floor(world.Y()));
	auto inCone = [&](int col) {
		REQUIRE(world.dungeon.PlacePlayer(col, row));
		return world.dungeon.InScreechCone(*bat, rules);
	};
	CHECK(inCone(8)); // 1.5 tiles ahead
	CHECK(inCone(9)); // 3 tiles
	CHECK_FALSE(inCone(10)); // 4 tiles along the row, a little more to the chest: out of range
	CHECK_FALSE(inCone(7)); // 1 tile ahead, the chest more than 30 degrees below its way
	CHECK_FALSE(inCone(6)); // right under it
	CHECK_FALSE(inCone(4)); // behind it

	// A wide, long cone, so only the wall (col 8) blocks it.
	const ScreechRules wide = {8.f, 120.f, 600, 1500, 6000, 0};
	SimWorld walled;
	const Monster* other = roostingBat(walled, "tests/levels/giant_bat_wall");
	REQUIRE(walled.dungeon.PlacePlayer(7, row));
	CHECK(walled.dungeon.InScreechCone(*other, wide)); // before the wall
	REQUIRE(walled.dungeon.PlacePlayer(10, row));
	CHECK_FALSE(walled.dungeon.InScreechCone(*other, wide)); // behind it
	REQUIRE(world.dungeon.PlacePlayer(10, row));
	CHECK(world.dungeon.InScreechCone(*bat, wide)); // there, without the wall
}

TEST_CASE("the giant bat stops, shakes through the tell, then the player in the cone is dazed") {
	SimWorld world;
	world.player.god = true;
	const Monster* bat = roostingBat(world, "tests/levels/giant_bat");
	REQUIRE(world.WalkTo(4.9f)); // in its sight: it wakes
	bool tell = false;
	float tellX = 0.f;
	for (int t = 0; t < 100 && !tell; t++) {
		world.Tick();
		tell = bat->Screeching();
		tellX = bat->CentreX();
	}
	REQUIRE(tell);
	CHECK(world.Saw(MonsterGiantBat, CreatureMove::Screech));
	CHECK_FALSE(world.player.Dazed()); // the tell first
	world.Wait(400);
	CHECK(bat->Screeching());
	CHECK(bat->CentreX() == doctest::Approx(tellX)); // still in the air
	for (int t = 0; t < 30 && bat->Screeching(); t++)
		world.Tick();
	CHECK_FALSE(bat->Screeching());
	CHECK(world.player.Dazed());
	CHECK(world.player.dazeMs > 1400);
}

TEST_CASE("a player who steps out of the cone in the tell is not dazed") {
	SimWorld world;
	world.player.god = true;
	const Monster* bat = roostingBat(world, "tests/levels/giant_bat");
	REQUIRE(world.WalkTo(4.9f));
	for (int t = 0; t < 100 && !bat->Screeching(); t++)
		world.Tick();
	REQUIRE(bat->Screeching());
	REQUIRE(world.WalkTo(3.f)); // back out of its sight and its cone's angle: behind and below it is safe
	for (int t = 0; t < 60 && bat->Screeching(); t++)
		world.Tick();
	CHECK_FALSE(world.player.Dazed());
}

TEST_CASE("dazed: half the walk speed, no sprint, no jump; it wears off") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/giant_bat"));
	const float walk = world.dungeon.PlayerWalkFactor();
	world.player.Daze(1500, world.events);
	CHECK(world.dungeon.PlayerWalkFactor() == doctest::Approx(walk * DAZE_WALK_FACTOR));
	CHECK_FALSE(world.dungeon.PlayerCanSprint());
	CHECK(world.dungeon.PlayerSprintMultiplier(SPRINT_MULTIPLIER) == 1.f);
	world.dungeon.StartJump(1);
	CHECK_FALSE(world.player.jump.jumping);
	CHECK(world.player.Daziness() == doctest::Approx(1.f));
	world.Wait(1500 + UPDATE_TICK_MS);
	CHECK_FALSE(world.player.Dazed());
	CHECK(world.player.Daziness() == 0.f);
	CHECK(world.dungeon.PlayerWalkFactor() == doctest::Approx(walk));
	CHECK(world.dungeon.PlayerCanSprint());
	world.dungeon.StartJump(1);
	CHECK(world.player.jump.jumping);
}

// tests/levels/bats_ladder: a ladder (col 6) up a shaft from the lower floor.
TEST_CASE("a dazed player on a ladder lets go and falls to the floor; they cannot climb until the daze is over") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/bats_ladder"));
	const float floor = world.Y();
	REQUIRE(world.WalkTo(6.45f));
	REQUIRE(world.Climb(1.f));
	REQUIRE(world.dungeon.PlayerOnLadder());
	world.player.Daze(1500, world.events);
	world.Wait(800);
	CHECK(world.Y() == doctest::Approx(floor));
	CHECK_FALSE(world.dungeon.PlayerOnLadder());
	CHECK_FALSE(world.Climb(0.5f)); // no grip
	world.Wait(1000);
	CHECK(world.Climb(1.f));
}

// tests/levels/shezmu: the Bat of Shezmu (col 8) on the ceiling.
TEST_CASE("Shezmu's bite heals him by all its damage while the player is dazed, by his share otherwise") {
	auto healShare = [](bool dazed) {
		SimWorld world;
		REQUIRE(world.Load("tests/levels/shezmu"));
		world.player.stats.AddXP(10000000, world.events); // lives through his bites
		const auto row = static_cast<int>(std::floor(world.Y()));
		REQUIRE(world.dungeon.SpawnMonster(8, row));
		world.Tick();
		world.dungeon.HurtBoss(300);
		REQUIRE(world.WalkTo(7.f));
		for (int t = 0; t < 1500; t++) {
			if (dazed)
				world.player.dazeMs = 1000; // the daze kept up, no screech needed
			const int hp = world.Hp(), boss = world.dungeon.BossHealth();
			world.Tick();
			if (world.player.Dazed() != dazed) // his screech
				continue;
			const int lost = hp - world.Hp(), healed = world.dungeon.BossHealth() - boss;
			if (lost > 10 && healed > 0)
				return static_cast<float>(healed) / static_cast<float>(lost);
		}
		FAIL("no bite");
		return 0.f;
	};
	CHECK(healShare(true) == doctest::Approx(1.f).epsilon(0.05));
	CHECK(healShare(false) == doctest::Approx(monsterKind(MonsterVampireBat)->boss.lifeStealPct / 100.f).epsilon(0.1));
}
