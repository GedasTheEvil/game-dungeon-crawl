// Blinding spit (plan giant-cobra-blinding-spit): the giant cobra's spit blinds with half its hits, Apep's fan of three
// with each; the blind clears over its last BLIND_CLEAR_MS, twice as fast in water.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

TEST_CASE("the giant cobra's spit blinds half the time, Apep's fan of three always and longer; the cobra's never") {
	const MonsterKind& cobra = *monsterKind(MonsterCobra);
	const MonsterKind& giant = *monsterKind(MonsterGiantCobra);
	const MonsterKind& apep = *monsterKind(MonsterApep);
	REQUIRE(cobra.ranged.has_value());
	REQUIRE(giant.ranged.has_value());
	REQUIRE(apep.ranged.has_value());
	CHECK(cobra.ranged->blindMs == 0);
	CHECK(giant.ranged->blindMs == 2000);
	CHECK(giant.ranged->blindPercent == 50);
	CHECK(giant.ranged->fan == 1);
	CHECK(apep.kin == MonsterCobra);
	CHECK(apep.ranged->blindMs == 3000);
	CHECK(apep.ranged->blindPercent == 100);
	CHECK(apep.ranged->fan == 3);
}

TEST_CASE("a blind lasts its time, clears over the last BLIND_CLEAR_MS, twice as fast in water") {
	WorldEvents events;
	Player player;
	player.stats.AddXP(0, events);
	player.Blind(2000, events);
	CHECK(player.blindMs == 2000);
	player.Blind(1000, events); // a shorter one keeps the longer
	CHECK(player.blindMs == 2000);
	CHECK(player.Blindness() == doctest::Approx(1.f));
	auto run = [&player](int ms) {
		for (int t = 0; t < ms / UPDATE_TICK_MS; t++)
			player.UpdateBlind();
	};
	run(2000 - BLIND_CLEAR_MS);
	CHECK(player.Blindness() == doctest::Approx(1.f));
	run(BLIND_CLEAR_MS / 2);
	CHECK(player.Blindness() > 0.4f);
	CHECK(player.Blindness() < 0.6f);
	run(BLIND_CLEAR_MS);
	CHECK(player.Blindness() == 0.f);

	player.Blind(2000, events);
	player.inWater = true; // wading: as UpdateBurning notes it
	run(1000 + UPDATE_TICK_MS);
	CHECK(player.blindMs == 0);
	player.Blind(2000, events);
	player.Reanimate();
	CHECK(player.blindMs == 0);
}

TEST_CASE("the giant cobra's spit blinds on some hits and not on others, the bite never") {
	int blinded = 0, seen = 0;
	for (uint64_t seed = 1; seed <= 12; seed++) {
		SimWorld world;
		world.player.god = true;
		REQUIRE(world.Load("tests/levels/giant_cobra", seed));
		REQUIRE(world.WalkTo(world.X() + 4.9f)); // in its wake range: it rears and spits
		for (int t = 0; t < 600 && !world.player.stats.poison.Any(); t++)
			world.Tick();
		REQUIRE(world.player.stats.poison.Any());
		if (!world.Saw(MonsterGiantCobra, CreatureMove::AttackRanged))
			continue; // bitten first
		seen++;
		if (world.player.blindMs > 0) {
			blinded++;
			CHECK(world.Saw(MonsterGiantCobra, CreatureMove::Blind));
		}
	}
	CHECK(seen >= 6);
	CHECK(blinded > 0);
	CHECK(blinded < seen);
}

TEST_CASE("Apep spits a fan of three, each hit blinds") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/apep"));
	REQUIRE(world.WalkTo(9.5f));
	int shots = 0;
	for (int t = 0; t < 2000 && shots == 0; t++) {
		world.Tick();
		shots = world.dungeon.ShotCount();
	}
	CHECK(shots == 3);
	for (int t = 0; t < 300 && world.player.blindMs == 0; t++)
		world.Tick();
	CHECK(world.player.blindMs > 2000);
	CHECK(world.Saw(MonsterApep, CreatureMove::Blind));
}
