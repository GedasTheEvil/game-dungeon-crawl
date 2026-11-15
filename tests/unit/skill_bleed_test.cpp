// Skill Bleed (plan skill-bleed): with the skill, a weapon hit that does not kill makes the monster bleed (a weak bleed) at 20% of
// the hit's cut share (a boss half), scaled once more by the monster's cut rate; a bloodless kind never bleeds; a
// bleed kill is the player's.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
// `level` loaded, the player with (or without) the Bleed skill, up to the nearest monster (woken, in reach).
Monster*
readyMonster(SimWorld& world, const char* level, bool skill, bool boss, std::initializer_list<float> route = {}) {
	world.player.god = true;
	REQUIRE(world.Load(level));
	if (skill) {
		world.player.stats.heka.Earn(5);
		REQUIRE(world.player.stats.heka.Learn(SkillId::Bleed));
	}
	for (float x : route) // wakes what lurks or sleeps
		REQUIRE(world.WalkTo(x));
	world.Wait(3000);
	// The tests keep it alive by hand (RestoreHealth): the world gives only a const view. A boss: the level's boss, not
	// the minions round it.
	Monster* mon = nullptr;
	for (int i = 0; i < MAX_MONSTERS; i++) {
		auto* cand = const_cast<Monster*>(&world.dungeon.Monsters()[i]);
		if (cand->Active() && cand->Alive() && cand->Type()->isBoss() == boss && (!mon || cand->Col() > mon->Col()))
			mon = cand; // the farthest along: the others are in the way
	}
	REQUIRE(mon != nullptr);
	REQUIRE(world.WalkTo(mon->Left() - world.player.HalfWidth() - 0.1f));
	return mon;
}

// Hits of no damage with mix, the monster healed after each and its bleed let run out: how many started a bleed.
int bleedsIn(SimWorld& world, Monster& mon, const DamageMix& mix, int hits) {
	int bleeds = 0;
	for (int i = 0; i < hits; i++) {
		mon.RestoreHealth(mon.MaxHealth());
		REQUIRE(world.dungeon.AttackNearest(0, mix, 3.f, mon.CentreX() > world.X() ? 1 : -1));
		if (mon.Bleeds()) {
			bleeds++;
			world.Wait(BLEED_TIERS[0].durationMs + 2 * UPDATE_TICK_MS);
		} else
			world.Wait(2 * UPDATE_TICK_MS);
	}
	return bleeds;
}

// The chance a hit of mix makes mon bleed, in percent: the skill's own, then its cut rate, as the rules say.
int expectedPercent(const Monster& mon, int sharePercent) {
	int base = BLEED_CHANCE_PERCENT * sharePercent / 100;
	if (mon.Type()->isBoss())
		base = base * BOSS_BLEED_PERCENT / 100;
	return bleedChancePercent(base, mon.Type()->resist);
}
} // namespace

TEST_CASE("skill-bleed: a cutting hit bleeds at 20% of its cut share, a mace never, no skill never") {
	SimWorld world;
	Monster* mon = readyMonster(world, "tests/levels/giant_scorpion", true, false);
	REQUIRE_FALSE(mon->Type()->bloodless);
	REQUIRE_FALSE(mon->Type()->isBoss());
	const int sword = expectedPercent(*mon, 85);
	REQUIRE(sword > 0);
	const int hits = 300;
	const int bleeds = bleedsIn(world, *mon, {0, 85, 15}, hits);
	CHECK(bleeds >= hits * sword / 100 / 2);
	CHECK(bleeds <= hits * sword / 100 * 2);
	CHECK(bleedsIn(world, *mon, {100, 0, 0}, 100) == 0); // a mace

	SimWorld unskilled;
	Monster* other = readyMonster(unskilled, "tests/levels/giant_scorpion", false, false);
	CHECK(bleedsIn(unskilled, *other, {0, 100, 0}, 100) == 0);
}

TEST_CASE("skill-bleed: a boss bleeds at half the chance") {
	SimWorld world;
	Monster* boss = readyMonster(world, "tests/levels/scorpion_queen", true, true, {9.f});
	REQUIRE(boss->Type()->isBoss());
	const int half = expectedPercent(*boss, 100);
	CHECK(half == bleedChancePercent(BLEED_CHANCE_PERCENT / 2, boss->Type()->resist));
	REQUIRE(half > 0);
	const int hits = 300;
	const int bleeds = bleedsIn(world, *boss, {0, 100, 0}, hits);
	CHECK(bleeds >= hits * half / 100 / 2);
	CHECK(bleeds <= hits * half / 100 * 2);
}

TEST_CASE("skill-bleed: a bloodless kind never bleeds, however it is cut") {
	SimWorld world;
	Monster* mummy = readyMonster(world, "tests/levels/fire", true, false, {12.f, 9.5f});
	REQUIRE(mummy->Type()->bloodless);
	CHECK(bleedsIn(world, *mummy, {0, 100, 0}, 150) == 0);
}

TEST_CASE("skill-bleed: a monster that bleeds out from the player's cut gives the player the kill") {
	SimWorld world;
	Monster* mon = readyMonster(world, "tests/levels/giant_scorpion", true, false);
	for (int i = 0; i < 400 && !mon->Bleeds(); i++) {
		mon->RestoreHealth(mon->MaxHealth());
		REQUIRE(world.dungeon.AttackNearest(0, {0, 100, 0}, 3.f, mon->CentreX() > world.X() ? 1 : -1));
	}
	REQUIRE(mon->Bleeds());
	const double xp = world.player.stats.CurrentXP();
	mon->RestoreHealth(1);
	world.Wait(2500);
	CHECK_FALSE(mon->Alive());
	CHECK(world.player.stats.CurrentXP() > xp);
}
