// The pounce (plan monster-balance): the giant rat and the giant scarab leap at the player on their row from 2-3 tiles,
// the way clear, never from half water; the rat lands in bite reach, short of a player who steps towards it, the
// scarab in its dung ball's band. Khepri (the boss rule) from farther, more often.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
const Monster* firstMonster(const SimWorld& world) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active())
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

// Ticks until pred holds, at most ms.
template <typename Pred> bool waitFor(SimWorld& world, int ms, Pred pred) {
	for (int t = 0; t < ms / UPDATE_TICK_MS; t++) {
		if (pred())
			return true;
		world.Tick();
	}
	return pred();
}
} // namespace

TEST_CASE("the pouncers: the giant rat and the giant scarab; Khepri from farther and more often") {
	const MonsterKind& rat = *monsterKind(MonsterGiantRat);
	const MonsterKind& scarab = *monsterKind(MonsterGiantScarab);
	const MonsterKind& khepri = *monsterKind(MonsterBossScarab);
	REQUIRE(rat.pounce);
	REQUIRE(scarab.pounce);
	REQUIRE(khepri.pounce);
	CHECK_FALSE(monsterKind(MonsterRat)->pounce);
	CHECK_FALSE(monsterKind(MonsterScarab)->pounce);
	for (const MonsterKind* kind : {&rat, &scarab, &khepri})
		CHECK(leaps(kind->locomotion));
	CHECK(rat.pounce->minGap == 2.f);
	CHECK(rat.pounce->maxGap == 3.f);
	CHECK(rat.pounce->cooldownMs == 3000);
	CHECK(rat.pounce->landGap < MONSTER_BITE_REACH);
	// The scarab leaps from just beyond its ball's band into it: no gap between the two a player can keep it in.
	for (const MonsterKind* kind : {&scarab, &khepri}) {
		CHECK(kind->pounce->minGap > kind->roll->maxRange);
		CHECK(kind->pounce->minGap <= kind->roll->maxRange + 0.2f);
	}
	CHECK(scarab.pounce->landGap >= scarab.roll->minRange);
	CHECK(scarab.pounce->landGap <= scarab.roll->maxRange);
	CHECK(khepri.pounce->landGap <= khepri.roll->maxRange);
	CHECK(khepri.pounce->maxGap > scarab.pounce->maxGap);
	CHECK(khepri.pounce->cooldownMs < scarab.pounce->cooldownMs);
	CHECK(monsterKind(MonsterGiantScarab)->speed == 4.5f);
}

TEST_CASE("the giant rat pounces from 2-3 tiles and lands in bite reach; once a cooldown") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/pounce"));
	world.Tick();
	const Monster* rat = firstMonster(world);
	REQUIRE(rat != nullptr);
	CHECK(rat->PlayerGap(world.X()) > 3.f);
	REQUIRE(waitFor(world, 3000, [&] { return rat->pouncing(); }));
	const float from = rat->PlayerGap(world.X());
	CHECK(from >= 2.f);
	CHECK(from <= 3.f);
	CHECK(world.Saw(MonsterGiantRat, CreatureMove::Leap));
	REQUIRE(waitFor(world, MONSTER_JUMP_MS + 100, [&] { return !rat->jumping(); }));
	CHECK(rat->PlayerGap(world.X()) >= 0.f);
	CHECK(rat->PlayerGap(world.X()) < MONSTER_BITE_REACH);
	const int landed = GameClock::now();

	// The player backs off past its band and comes back into it: no new pounce until the cooldown is over.
	const int cooldown = monsterKind(MonsterGiantRat)->pounce->cooldownMs;
	world.player.god = true;
	REQUIRE(world.WalkTo(world.X() - 2.5f));
	CHECK_FALSE(waitFor(world, cooldown - (GameClock::now() - landed) - 2 * UPDATE_TICK_MS - MONSTER_JUMP_MS, [&] {
		return rat->pouncing();
	}));
}

TEST_CASE("a player who steps towards a pouncing rat: it lands short of them, not in them") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/pounce"));
	world.Tick();
	const Monster* rat = firstMonster(world);
	REQUIRE(rat != nullptr);
	REQUIRE(waitFor(world, 3000, [&] { return rat->pouncing(); }));
	for (int t = 0; rat->jumping() && t < 100; t++) {
		world.Step(1);
		world.Tick();
	}
	CHECK(rat->PlayerGap(world.X()) >= -0.01f);
}

TEST_CASE("no pounce over a trap: the rat leaps the spikes as a gap, then bites") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/pounce_spikes"));
	world.Tick();
	const Monster* rat = firstMonster(world);
	REQUIRE(rat != nullptr);
	bool pounced = false;
	REQUIRE(waitFor(world, 4000, [&] {
		pounced = pounced || rat->pouncing();
		return rat->jumping() && !rat->pouncing();
	}));
	CHECK_FALSE(pounced);
}

TEST_CASE("no pounce out of half water: the giant scarab in its pool stays there") {
	SimWorld world; // tests/levels/water_arrow: a giant scarab in a pool at cols 4-7
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/water_arrow"));
	world.Tick();
	const Monster* scarab = firstMonster(world);
	REQUIRE(scarab != nullptr);
	REQUIRE(world.WalkTo(2.8f));
	CHECK_FALSE(waitFor(world, 2000, [&] { return scarab->jumping(); }));
	CHECK(scarab->InWater());
}
