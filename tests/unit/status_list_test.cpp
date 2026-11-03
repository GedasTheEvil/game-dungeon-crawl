// The HUD's status cards (plan curse-blessing-cards): the running effects in one list, by kind, with their icons, pips
// and tints.
#include "../../external/doctest/doctest.h"
#include "../../src/entities/player_stats.h"
#include "../../src/ui/status_list.h"
#include "../../src/world/world_events.h"

using PlayerHud::Icon;

TEST_CASE("the status list orders a mixed set by kind: blessings, conditions, curses") {
	GameClock::enableVirtual();
	PlayerStats stats;
	WorldEvents events;
	// Laid in the opposite order to the display.
	stats.LayCurse(Curse::Fatigue, CurseGrade::Lesser, events);
	stats.LayCurse(Curse::Weakness, CurseGrade::Greater, events);
	stats.poison.Apply(PoisonTier::Medium);
	stats.poison.Apply(PoisonTier::Weak);
	PotionGain resist;
	resist.resistPercent = PotionEffect::GREATER_RESIST_PERCENT;
	stats.Drink(resist);

	const StatusList list = statusList(stats, 2000, 0);
	REQUIRE(list.count == 6);
	const Status* s = list.items.data();
	CHECK(s[0].kind == StatusKind::Blessing); // the resistance potion, greater: two pips
	CHECK(s[0].icon == Icon::Drop);
	CHECK(s[0].pips == 2);
	CHECK(s[0].leftMs == PotionEffect::RESIST_MS);
	CHECK(s[1].kind == StatusKind::Condition); // poison, weak first
	CHECK(s[1].icon == Icon::Drop);
	CHECK(s[1].pips == 1);
	CHECK(s[2].kind == StatusKind::Condition);
	CHECK(s[2].pips == 2);
	CHECK(s[1].tint.g != s[2].tint.g); // a tier colour each
	CHECK(s[3].kind == StatusKind::Condition); // burning
	CHECK(s[3].icon == Icon::Burning);
	CHECK(s[3].pips == 0);
	CHECK(s[3].leftMs == 2000);
	CHECK(s[4].kind == StatusKind::Curse); // curses in Curse order: weakness (greater) before fatigue
	CHECK(s[4].icon == Icon::FirstCurse);
	CHECK(s[4].pips == 2);
	CHECK(s[5].kind == StatusKind::Curse);
	CHECK(static_cast<int>(s[5].icon) == static_cast<int>(Icon::FirstCurse) + static_cast<int>(Curse::Fatigue));
	CHECK(s[5].pips == 1);
}

TEST_CASE("the status list: Hapi and Isis are blessings, daze a condition, nothing running is empty") {
	GameClock::enableVirtual();
	PlayerStats stats;
	WorldEvents events;
	CHECK(statusList(stats, 0, 0).count == 0);

	PotionGain water;
	water.waterBlessing = true;
	stats.Drink(water);
	stats.Pray(events);
	const StatusList list = statusList(stats, 0, 1500);
	REQUIRE(list.count == 3);
	CHECK(list.items[0].kind == StatusKind::Blessing); // Hapi before Isis
	CHECK(list.items[0].icon == Icon::Drop);
	CHECK(list.items[1].kind == StatusKind::Blessing);
	CHECK(list.items[1].icon == Icon::IsisWard);
	CHECK(list.items[2].kind == StatusKind::Condition);
	CHECK(list.items[2].icon == Icon::Daze);
}

TEST_CASE("every effect at once fits the list") {
	GameClock::enableVirtual();
	PlayerStats stats;
	WorldEvents events;
	for (int c = 0; c < CURSE_COUNT; c++)
		stats.LayCurse(static_cast<Curse>(c), CurseGrade::Lesser, events);
	for (int t = 0; t < POISON_TIER_COUNT; t++)
		stats.poison.Apply(static_cast<PoisonTier>(t));
	CHECK(statusList(stats, 1000, 1000).count == 2 + CURSE_COUNT + POISON_TIER_COUNT);
}
