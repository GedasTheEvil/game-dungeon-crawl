// A monster that walks into half water splashes (plan monster-water-splash): a big body a Splash, a small one a Wade;
// one that already stands in it, or spawns in it, makes none.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
int count(const SimWorld& world, WorldSound sound) {
	return static_cast<int>(std::count_if(world.said.begin(), world.said.end(), [&](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Sound && e.sound == sound;
	}));
}

// Loads a level of tests/levels/splash_*: the walker comes at the god-mode player through the pool; runs 20 s.
void walkThroughPool(SimWorld& world, const char* level) {
	world.player.god = true;
	REQUIRE(world.Load(level));
	for (int t = 0; t < 20000 / UPDATE_TICK_MS; t++)
		world.Tick();
}
} // namespace

TEST_CASE("a big monster walking into half water splashes once, a small one only wades") {
	SimWorld big;
	walkThroughPool(big, "tests/levels/splash_giant_scorpion");
	CHECK(count(big, WorldSound::Splash) == 1);
	CHECK(count(big, WorldSound::Wade) == 0);

	SimWorld small;
	walkThroughPool(small, "tests/levels/splash_rat");
	CHECK(count(small, WorldSound::Splash) == 0);
	CHECK(count(small, WorldSound::Wade) == 1);
}

TEST_CASE("a monster that starts in the water, or lies in it, makes no splash") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/hippo"));
	world.Wait(3000);
	CHECK(count(world, WorldSound::Splash) == 0);
	CHECK(count(world, WorldSound::Wade) == 0);
}

TEST_CASE("Monster::SetInWater reports only the entry from dry cells") {
	Monster mon;
	CHECK_FALSE(mon.SetInWater(true, true, 0.f)); // the first tick after a spawn
	CHECK_FALSE(mon.SetInWater(true, true, 0.f));
	CHECK_FALSE(mon.SetInWater(false, false, 0.f));
	CHECK(mon.SetInWater(true, false, 0.f));
	CHECK_FALSE(mon.SetInWater(true, true, 0.f));
}
