// The baboon of the Lake of Fire (plan baboon): a kiter that hurls burning coals and bites when cornered.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
// The level loaded, the baboon on it: it has come up to its band and stands there, or has thrown already.
const Monster& baboonIn(SimWorld& world, const char* level, int waitMs = 6000) {
	REQUIRE(world.Load(level));
	world.Wait(waitMs);
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active())
			return world.dungeon.Monsters()[i];
	FAIL("no baboon");
	return world.dungeon.Monsters()[0];
}
} // namespace

TEST_CASE("the baboon is a kiter, a climber, its coal burns; resists fire, weak to pierce") {
	const MonsterKind& kind = *monsterKind(MonsterBaboon);
	REQUIRE(kind.ranged.has_value());
	REQUIRE(kind.kite.has_value());
	CHECK(kind.ranged->burns());
	CHECK(kind.ranged->ownClip);
	CHECK(kind.kite->far <= kind.ranged->range);
	CHECK(kind.kite->backOffSpeed > 19.f); // faster than the player's walk
	CHECK(kind.kite->backOffSpeed < 57.f); // slower than their sprint
	CHECK(kind.climbs);
	CHECK(kind.resist[FIRE] < NORMAL);
	CHECK(kind.resist[static_cast<size_t>(DamageType::Pierce)] > NORMAL);
	CHECK(&ClipFilesOf(kind) == &THROWER_CLIPS);
}

TEST_CASE("a baboon comes up to its band and throws: the coal sets the player burning") {
	SimWorld world;
	world.player.god = true;
	const Monster& baboon = baboonIn(world, "tests/levels/baboon", UPDATE_TICK_MS);
	bool burnt = false;
	for (int t = 0; t < 6000 && !burnt; t += UPDATE_TICK_MS) {
		world.Tick();
		burnt = world.player.burning.Any();
	}
	CHECK(burnt);
	CHECK(world.Saw(MonsterBaboon, CreatureMove::AttackRanged));
	CHECK(baboon.PlayerGap(world.X()) <= monsterKind(MonsterBaboon)->ranged->range);
	CHECK(baboon.PlayerGap(world.X()) > MONSTER_BITE_REACH);
}

TEST_CASE("the player in half water: the coal hits, but the water puts the fire out") {
	SimWorld world;
	world.player.stats.AddArmor(5);
	REQUIRE(world.Load("tests/levels/baboon_water"));
	const int hp = world.Hp();
	bool burnt = false;
	for (int t = 0; t < 6000 && world.Hp() == hp; t += UPDATE_TICK_MS) {
		world.Tick();
		burnt = burnt || world.player.burning.Any();
	}
	CHECK(world.Hp() < hp);
	CHECK(world.Saw(MonsterBaboon, CreatureMove::AttackRanged));
	world.Wait(200);
	CHECK_FALSE(burnt);
	CHECK_FALSE(world.player.burning.Any());
	CHECK(world.Told("The water puts the fire out"));
}

TEST_CASE("the player walks at a baboon: it backs off, stays in its band and goes on throwing") {
	SimWorld world;
	world.player.god = true;
	const Monster& baboon = baboonIn(world, "tests/levels/baboon");
	const float from = baboon.CentreX();
	const float far = monsterKind(MonsterBaboon)->kite->far;
	float nearest = 99.f, farthest = 0.f;
	int throws = 0;
	bool wasThrowing = false;
	for (int t = 0; t < 10000; t += UPDATE_TICK_MS) {
		world.Step(1);
		world.Tick();
		nearest = std::min(nearest, baboon.PlayerGap(world.X()));
		farthest = std::max(farthest, baboon.PlayerGap(world.X()));
		throws += baboon.AttackingRanged() && !wasThrowing ? 1 : 0;
		wasThrowing = baboon.AttackingRanged();
	}
	CHECK(baboon.CentreX() > from + 5.f); // backed off along the row
	CHECK(nearest > MONSTER_BITE_REACH); // walking never catches it
	CHECK(farthest <= far + 0.1f); // never past its range
	CHECK(throws >= 2);
	CHECK(world.Saw(MonsterBaboon, CreatureMove::Kite));
}

TEST_CASE("a baboon with a wall behind it bites the player in reach") {
	SimWorld world;
	world.player.god = true;
	const Monster& baboon = baboonIn(world, "tests/levels/baboon_wall");
	bool bit = false;
	for (int t = 0; t < 14000 && !bit; t += UPDATE_TICK_MS) {
		if (baboon.PlayerGap(world.X()) > MONSTER_BITE_REACH)
			world.Step(1);
		world.Tick();
		bit = baboon.Biting();
	}
	CHECK(bit);
	CHECK(baboon.CentreX() < 11.f);
}

TEST_CASE("cornered, a baboon closes in on the player standing nearer than its band and bites") {
	SimWorld world;
	world.player.god = true;
	const Monster& baboon = baboonIn(world, "tests/levels/baboon_wall");
	const float near = monsterKind(MonsterBaboon)->kite->near;
	while (baboon.PlayerGap(world.X()) > near - 0.15f) { // up to it, into the corner, then stand
		world.Step(1);
		world.Tick();
	}
	REQUIRE(baboon.PlayerGap(world.X()) > MONSTER_BITE_REACH);
	bool bit = false;
	for (int t = 0; t < 4000 && !bit; t += UPDATE_TICK_MS) {
		world.Tick();
		bit = baboon.Biting();
	}
	CHECK(bit);
}

TEST_CASE("the player sprints at a baboon: they catch it") {
	SimWorld world;
	world.player.god = true;
	const Monster& baboon = baboonIn(world, "tests/levels/baboon");
	world.player.stats.SetSprintRequested(true);
	int ms = 0;
	for (; ms < 4000 && baboon.PlayerGap(world.X()) > MONSTER_BITE_REACH; ms += UPDATE_TICK_MS) {
		world.Step(1);
		world.Tick();
	}
	CHECK(ms < 4000);
}
