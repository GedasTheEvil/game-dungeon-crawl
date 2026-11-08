// A shooter under a step (plan anubis-stuck-under-step): an Anubis that cannot bite the player up on a step backs off
// until its bolt's line clears the step, then shoots; on tests/levels/anubis_step and anubis_step_boss (their .txt
// draw them): a step three quarters high at cols 4 and 5, its face at x 6, the Anubis coming from col 8.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include <cmath>

namespace {
constexpr int ROW = 2, MONSTER_COL = 8, STEP_COL = 5;
constexpr float FACE = 6.f;

const Monster* spawn(SimWorld& world) {
	(void)world.dungeon.SpawnMonster(MONSTER_COL, ROW);
	for (int i = 0; i < MAX_MONSTERS; i++) {
		const Monster& mon = world.dungeon.Monsters()[i];
		if (mon.Active() && mon.Col() == MONSTER_COL && mon.SpawnRow() == ROW)
			return &mon;
	}
	return nullptr;
}

// The player up on the step at x, sturdy, the Anubis spawned and given the time to come up (its bolt on the way may
// hit). crouch: down, under the boss's high swing.
const Monster* setUp(SimWorld& world, const char* level, float x, bool crouch = false) {
	REQUIRE(world.Load(level));
	world.player.stats.AddXP(1000000, world.events);
	REQUIRE(world.dungeon.PlacePlayer(STEP_COL, ROW));
	world.Tick();
	REQUIRE(world.WalkTo(x));
	world.player.crouchHeld = crouch;
	const Monster* mon = spawn(world);
	REQUIRE(mon != nullptr);
	world.Wait(5000);
	return mon;
}

// From now, ticks until a fire bolt sets the player burning (a hit, not a splat on the step), at most ms.
bool boltHits(SimWorld& world, int ms) {
	world.player.stats.vitals.burning.Douse();
	for (int t = 0; t < ms && !world.player.stats.vitals.burning.Any(); t += UPDATE_TICK_MS)
		world.Tick();
	return world.player.stats.vitals.burning.Any();
}
} // namespace

TEST_CASE("an Anubis under a step over its swing backs off until its bolt clears the step, and the bolt hits") {
	SimWorld world;
	const Monster* anubis = setUp(world, "tests/levels/anubis_step", 5.9f); // at the edge, a spear thrust from it
	CHECK(boltHits(world, 8000));
	CHECK(anubis->Left() > FACE + 0.3f); // stepped back from the face, where its bolt hit the step's corner
	CHECK(anubis->PlayerGap(world.X()) <= monsterKind(MonsterAnubis)->ranged->range);
	const float stood = anubis->CentreX();
	world.Wait(4000); // lined up, it holds there for the next bolt, not back under the step
	CHECK(std::fabs(anubis->CentreX() - stood) < 0.15f);
}

TEST_CASE("an Anubis that finds no line in its range backs off out of a spear's reach and waits") {
	SimWorld world;
	const Monster* anubis = setUp(world, "tests/levels/anubis_step", 5.5f); // back from the edge
	CHECK_FALSE(boltHits(world, 8000)); // its bolts splat on the step
	const float range = monsterKind(MonsterAnubis)->ranged->range;
	CHECK(anubis->PlayerGap(world.X()) > weaponReach(ItemKind::Spear));
	CHECK(anubis->PlayerGap(world.X()) > range - 0.2f); // at its range's edge, not under the step (gap 0.5)
	CHECK(anubis->PlayerGap(world.X()) <= range);
}

TEST_CASE("the Anubis boss, its swing ducked on the step, backs off faster and its bolt hits") {
	CHECK(LINE_UP_BOSS_PACE > 1.f);
	SimWorld world;
	const Monster* boss = setUp(world, "tests/levels/anubis_step_boss", 5.9f, true);
	REQUIRE(boss->Type()->isBoss());
	CHECK(boltHits(world, 8000));
	CHECK(boss->Left() > FACE + 0.3f);
}

TEST_CASE("in open ground an Anubis still walks up and swings, no back-off") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/anubis_step"));
	world.player.stats.AddXP(1000000, world.events);
	REQUIRE(world.dungeon.PlacePlayer(7, ROW)); // on the floor between the step and the Anubis
	world.Tick();
	const Monster* anubis = spawn(world);
	REQUIRE(anubis != nullptr);
	const int hp = world.Hp();
	world.Wait(6000);
	CHECK(anubis->PlayerGap(world.X()) <= MONSTER_BITE_REACH + 0.01f);
	CHECK(world.Hp() < hp);
}
