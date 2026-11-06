// The HUD's status cards (plan curse-blessing-cards): the running effects in one list, by kind, with their icons, pips
// and tints.
#include "../../external/doctest/doctest.h"
#include "../../src/level/player_stats.h"
#include "../../src/level/status_list.h"
#include "../../src/level/world_events.h"

using PlayerHud::Icon;

TEST_CASE("the status list orders a mixed set by kind: blessings, conditions, curses") {
	GameClock::enableVirtual();
	PlayerStats stats;
	WorldEvents events;
	// Laid in the opposite order to the display.
	stats.LayCurse(Curse::Fatigue, CurseGrade::Lesser, events);
	stats.LayCurse(Curse::Weakness, CurseGrade::Greater, events);
	stats.vitals.poison.Apply(PoisonTier::Medium);
	stats.vitals.poison.Apply(PoisonTier::Weak);
	PotionGain resist;
	resist.resistPercent = PotionEffect::GREATER_RESIST_PERCENT;
	stats.Drink(resist);

	stats.vitals.burning.Ignite(5.0, 2000);
	const StatusList list = statusList(stats);
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
	CHECK(statusList(stats).count == 0);

	PotionGain water;
	water.waterBlessing = true;
	stats.Drink(water);
	stats.Pray(God::Isis, events);
	stats.vitals.KnockOut(1500);
	const StatusList list = statusList(stats);
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
		stats.vitals.poison.Apply(static_cast<PoisonTier>(t));
	stats.vitals.burning.Ignite(5.0, 1000);
	stats.vitals.KnockOut(1000);
	CHECK(statusList(stats).count == 2 + CURSE_COUNT + POISON_TIER_COUNT);
}

TEST_CASE("monster-status-icons: a monster's row is its conditions in the player's order, its terrain mood last") {
	GameClock::enableVirtual();
	Vitals vitals{10};
	CHECK(monsterStatuses(vitals, TerrainMood::Neutral).count == 0);
	vitals.poison.Apply(PoisonTier::Strong);
	vitals.burning.Ignite(5.0, 2000);
	vitals.KnockOut(1500);
	const StatusList hostile = monsterStatuses(vitals, TerrainMood::Hostile);
	REQUIRE(hostile.count == 4);
	CHECK(hostile.items[0].icon == Icon::Drop); // the strong tier, as the player's card
	CHECK(hostile.items[0].pips == 3);
	CHECK(hostile.items[1].icon == Icon::Burning);
	CHECK(hostile.items[2].icon == Icon::Daze); // stunned: the player's daze stars
	CHECK(hostile.items[3].kind == StatusKind::Condition);
	CHECK(hostile.items[3].icon == Icon::TerrainHostile);
	const StatusList native = monsterStatuses(Vitals{10}, TerrainMood::Native);
	REQUIRE(native.count == 1);
	CHECK(native.items[0].kind == StatusKind::Blessing);
	CHECK(native.items[0].icon == Icon::TerrainNative);
	// The player's row holds the same conditions, through the same function.
	PlayerStats stats;
	stats.vitals.poison.Apply(PoisonTier::Strong);
	stats.vitals.burning.Ignite(5.0, 2000);
	stats.vitals.KnockOut(1500);
	const StatusList player = statusList(stats);
	REQUIRE(player.count == 3);
	for (int i = 0; i < 3; i++)
		CHECK(player.items[static_cast<size_t>(i)].icon == hostile.items[static_cast<size_t>(i)].icon);
}
