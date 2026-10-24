// Kiters (plan monster-kiting): keep the player at the range of their shot, back off, fight when cornered.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
// No kind kites yet (the baboon will), so the tests make the Anubis one: its band 1.5 to 3 tiles, its shot reaching
// 3.2, backing off at 1.3 tiles a second. Its bolt and blow take 1 HP and lay no curse, so the player lives.
const MonsterTypes& kiterTypes() {
	static const MonsterTypes types = [] {
		MonsterTypes t = SimWorld::monsterTypes();
		MonsterType& anubis = t[MonsterAnubis];
		anubis.kite = KiteRules{1.5f, 3.f, 25.f};
		anubis.ranged->range = 3.2f;
		anubis.ranged->damage = 1;
		anubis.ranged->mix = std::nullopt;
		anubis.damage = 1;
		anubis.meleeCurses = {};
		anubis.rangedCurses = {};
		return t;
	}();
	return types;
}

// The level loaded, the kiter on it: it has come up to its band and stands there.
const Monster& kiterIn(SimWorld& world, const char* level) {
	world.dungeon.Link(
		{&world.player,
		 &world.journal,
		 &world.items,
		 &world.random,
		 nullptr,
		 &world.events,
		 &kiterTypes(),
		 &world.inscriptions,
		 &world.names}
	);
	REQUIRE(world.Load(level));
	world.Wait(6000);
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active())
			return world.dungeon.Monsters()[i];
	FAIL("no kiter");
	return world.dungeon.Monsters()[0];
}

// The player walks at the kiter for ms (or until in its bite reach). True: it started a bite on the way.
bool walkAt(SimWorld& world, const Monster& kiter, int ms) {
	bool bit = false;
	for (int t = 0; t < ms; t += UPDATE_TICK_MS) {
		if (kiter.PlayerGap(world.X()) > MONSTER_BITE_REACH)
			world.Step(1);
		world.Tick();
		bit = bit || kiter.Biting();
	}
	return bit;
}
} // namespace

TEST_CASE("a kiter comes up to its band, stands there and shoots") {
	SimWorld world;
	const Monster& kiter = kiterIn(world, "tests/levels/kite_open");
	CHECK(kiter.PlayerGap(world.X()) <= 3.f);
	CHECK(kiter.PlayerGap(world.X()) > 2.5f);
	CHECK(world.Saw(MonsterAnubis, CreatureMove::AttackRanged));
	CHECK(world.Hp() < 50);

	const float x = kiter.CentreX();
	world.Wait(3000); // the player stands still in its band: it does not move
	CHECK(kiter.CentreX() == x);
}

TEST_CASE("the player walks at a kiter: it backs off, stays in its band and goes on shooting") {
	SimWorld world;
	const Monster& kiter = kiterIn(world, "tests/levels/kite_open");
	const float from = kiter.CentreX();
	const int hp = world.Hp();
	float nearest = 99.f, farthest = 0.f;
	for (int t = 0; t < 8000; t += UPDATE_TICK_MS) {
		world.Step(1);
		world.Tick();
		nearest = std::min(nearest, kiter.PlayerGap(world.X()));
		farthest = std::max(farthest, kiter.PlayerGap(world.X()));
	}
	CHECK(kiter.CentreX() > from + 5.f); // backed off along the row
	CHECK(nearest > MONSTER_BITE_REACH); // walking never catches it
	CHECK(farthest <= 3.f); // never past its range
	CHECK(world.Hp() < hp); // shot at on the way
	CHECK(!world.Saw(MonsterAnubis, CreatureMove::Curse));
}

TEST_CASE("a kiter with a wall behind it fights the player in reach") {
	SimWorld world;
	const Monster& kiter = kiterIn(world, "tests/levels/kite_wall");
	CHECK(walkAt(world, kiter, 14000));
	CHECK(kiter.CentreX() < 11.f);
}

TEST_CASE("a kiter does not back off down a drop shaft: it stops at the edge and fights") {
	SimWorld world;
	const Monster& kiter = kiterIn(world, "tests/levels/kite_drop");
	const int row = kiter.Row();
	CHECK(walkAt(world, kiter, 14000));
	CHECK(kiter.Row() == row);
	CHECK(kiter.CentreX() < 11.f);
}

TEST_CASE("the player sprints at a kiter: they catch it") {
	SimWorld world;
	const Monster& kiter = kiterIn(world, "tests/levels/kite_open");
	world.player.stats.SetSprintRequested(true);
	int ms = 0;
	for (; ms < 4000 && kiter.PlayerGap(world.X()) > MONSTER_BITE_REACH; ms += UPDATE_TICK_MS) {
		world.Step(1);
		world.Tick();
	}
	CHECK(ms < 4000);
}
