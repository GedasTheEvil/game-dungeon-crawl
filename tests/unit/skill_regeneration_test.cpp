// Touch of Imhotep (plan skill-regeneration): a Heka skill that regenerates HP under the safe rule of plan
// regen-safe-rule. Rank I 1 HP / 5 s, rank II 0.25% of max HP / s; stacks with the amulet and the garnet rings.
#include "../../external/doctest/doctest.h"
#include "../../src/level/rules/player_stats.h"
#include "../../src/level/rules/skills.h"
#include "../../src/level/rules/vitals.h"
#include <algorithm>

namespace {
// A player with the skill at the rank, `lost` HP down, the loss's pause over.
PlayerStats hurtWith(int rank, int maxHpPercent, int lost) {
	PlayerStats stats;
	stats.AddMaxHP(maxHpPercent);
	stats.heka.Earn(5);
	stats.heka.Earn(10);
	REQUIRE(stats.Learn(SkillId::TouchOfImhotep));
	if (rank == 2)
		REQUIRE(stats.Upgrade(SkillId::TouchOfImhotep));
	stats.LoseHP(lost);
	GameClock::advance(REGEN_DAMAGE_PAUSE_MS);
	return stats;
}

// Ticks for `ms`, the game clock moving with them.
void tick(PlayerStats& stats, int ms) {
	for (int t = 0; t < ms; t += UPDATE_TICK_MS) {
		stats.Regenerate(UPDATE_TICK_MS);
		GameClock::advance(UPDATE_TICK_MS);
	}
}
} // namespace

TEST_CASE("plan skill-regeneration: rank I heals 1 HP every 5 s, none for 5 s after a loss or while poisoned") {
	PlayerStats stats = hurtWith(1, 0, 20);
	const int hp = stats.CurrentHP();
	tick(stats, REGEN_SKILL_MS_PER_HP - UPDATE_TICK_MS);
	CHECK(stats.CurrentHP() == hp);
	tick(stats, UPDATE_TICK_MS);
	CHECK(stats.CurrentHP() == hp + 1);
	tick(stats, 4 * REGEN_SKILL_MS_PER_HP);
	CHECK(stats.CurrentHP() == hp + 5);
	stats.LoseHP(1); // a hit: the pause
	const int hit = stats.CurrentHP();
	tick(stats, REGEN_DAMAGE_PAUSE_MS - UPDATE_TICK_MS);
	CHECK(stats.CurrentHP() == hit);
	tick(stats, REGEN_SKILL_MS_PER_HP + UPDATE_TICK_MS);
	CHECK(stats.CurrentHP() == hit + 1);
	stats.vitals.poison.Apply(PoisonTier::Weak);
	const int poisoned = stats.CurrentHP();
	tick(stats, 3 * REGEN_SKILL_MS_PER_HP);
	CHECK(stats.CurrentHP() == poisoned);
}

TEST_CASE(
	"plan skill-regeneration: rank II at 400 max HP heals 1 HP a second, at 100 max 1 HP in 4 s, in place of rank I"
) {
	PlayerStats big = hurtWith(2, 700, 100);
	REQUIRE(big.CurrentMaxHP() == 400);
	const int hp = big.CurrentHP();
	tick(big, 10000);
	CHECK(big.CurrentHP() == hp + 10);
	PlayerStats small = hurtWith(2, 100, 50); // 100 max HP
	REQUIRE(small.CurrentMaxHP() == 100);
	const int low = small.CurrentHP();
	tick(small, 20000);
	CHECK(small.CurrentHP() == low + 5);
	PlayerStats one = hurtWith(1, 100, 50);
	tick(one, 20000);
	CHECK(one.CurrentHP() == low + 4); // rank I: 1 in 5 s; rank II's 5 is not on top of it
}

TEST_CASE("plan skill-regeneration: the skill, the amulet and the garnet rings add; never above max HP") {
	PlayerStats stats = hurtWith(1, 0, 40);
	AmuletBonus amulet;
	amulet.regenHpPerSecond = 2;
	stats.Wear(amulet, true);
	RingBonus rings;
	rings.regenHpPerHour = 3600; // 1 HP a second
	stats.WearRings(rings);
	const int hp = stats.CurrentHP();
	tick(stats, 10000);
	CHECK(stats.CurrentHP() == hp + 20 + 10 + 2); // the amulet 2 a second, the rings 1, the skill 2 in 10 s
	tick(stats, 60000);
	CHECK(stats.CurrentHP() == stats.CurrentMaxHP());
}
