// The baboon's coal may daze the player (plan baboon-coal-stun): 3 s, one hit in five; Babi's, the boss, longer and
// likelier. A dazed player rolls nothing and is not dazed longer.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/level/gameplay_shared.h"

TEST_CASE("the coal's daze is data: the baboon 3 s, Babi's stronger; no other thrower dazes") {
	const MonsterKind& baboon = *monsterKind(MonsterBaboon);
	const MonsterKind& babi = *monsterKind(MonsterBabi);
	REQUIRE(baboon.ranged.has_value());
	REQUIRE(babi.ranged.has_value());
	CHECK(baboon.ranged->dazeMs == 3000);
	CHECK(baboon.ranged->dazePercent == 20);
	CHECK(babi.kin == MonsterBaboon);
	CHECK(babi.ranged->dazeMs > baboon.ranged->dazeMs);
	CHECK(babi.ranged->dazePercent > baboon.ranged->dazePercent);
	CHECK(monsterKind(MonsterCobra)->ranged->dazeMs == 0);
}

TEST_CASE("a coal hit that rolls dazes 3 s; a second hit in the daze rolls nothing and leaves it as is") {
	WorldEvents events;
	Player player;
	player.stats.AddXP(0, events);
	Rng rng(1);
	CHECK_FALSE(player.DazeByHit(3000, 0, events, rng)); // 0%: never
	CHECK_FALSE(player.Dazed());
	CHECK(player.DazeByHit(3000, 100, events, rng)); // the roll forced
	CHECK(player.Dazed());
	CHECK(player.stats.vitals.OutLeftMs() == 3000);
	Rng copy = rng;
	CHECK_FALSE(player.DazeByHit(9000, 100, events, rng));
	CHECK(player.stats.vitals.OutLeftMs() == 3000);
	CHECK(rng.next() == copy.next()); // sanity: same stream
	Rng spent = rng; // the failed call below must not move it
	player.DazeByHit(9000, 100, events, rng);
	CHECK(rng.next() == spent.next());
}

TEST_CASE("over a minute of baboon coals the player is dazed on some seeds, and dazed turns into 3 s") {
	int dazed = 0, hit = 0;
	for (uint64_t seed = 1; seed <= 8; seed++) {
		SimWorld world;
		world.player.god = true;
		REQUIRE(world.Load("tests/levels/baboon", seed));
		bool seen = false;
		for (int t = 0; t < 60000 && !seen; t += UPDATE_TICK_MS) {
			world.Tick();
			seen = world.player.Dazed();
		}
		hit += world.Saw(MonsterBaboon, CreatureMove::AttackRanged) ? 1 : 0;
		if (seen) {
			dazed++;
			CHECK(world.player.stats.vitals.OutLeftMs() <= 3000);
		}
	}
	CHECK(hit == 8);
	CHECK(dazed >= 4);
}
