// The shots amulet (plan amulet-more-shots) and the quick shots amulet (plan amulet-quick-shots): more shots in every
// ranged weapon, and the shots back sooner.
#include "../../external/doctest/doctest.h"
#include "../../src/entities/player_stats.h"
#include "sim_world.h"
#include <string>

namespace {
constexpr ItemKind SHOTS[AMULET_TIER_COUNT] =
	{ItemKind::ShotsLesser, ItemKind::ShotsMinor, ItemKind::ShotsNormal, ItemKind::ShotsGrand};
constexpr ItemKind QUICK[AMULET_TIER_COUNT] =
	{ItemKind::QuickShotsLesser, ItemKind::QuickShotsMinor, ItemKind::QuickShotsNormal, ItemKind::QuickShotsGrand};
} // namespace

TEST_CASE("the shot amulets: four tiers, +25 / 50 / 75 / 100%, after the piercing amulets") {
	const int percent[AMULET_TIER_COUNT] = {25, 50, 75, 100};
	for (size_t tier = 0; tier < AMULET_TIER_COUNT; tier++) {
		CHECK(amuletBonus(SHOTS[tier]).shotsPercent == percent[tier]);
		CHECK(amuletBonus(SHOTS[tier]).shotRecoveryPercent == 0);
		CHECK(amuletBonus(QUICK[tier]).shotRecoveryPercent == percent[tier]);
		CHECK(amuletBonus(QUICK[tier]).shotsPercent == 0);
	}
	CHECK(amuletBonus(ItemKind::StrengthGrand).shotsPercent == 0);
	CHECK(*amuletKind(AmuletType::Shots, AmuletTier::Lesser) == ItemKind::ShotsLesser);
	CHECK(*amuletKind(AmuletType::QuickShots, AmuletTier::Grand) == ItemKind::QuickShotsGrand);
	CHECK(static_cast<int>(ItemKind::ShotsLesser) == static_cast<int>(ItemKind::PiercingGrand) + 1);
	CHECK(std::string(itemText(ItemKind::ShotsMinor).name) == "Minor Satet's Quiver");
	CHECK(std::string(itemText(ItemKind::QuickShotsGrand).name) == "Grand Bastet's Cat");
	CHECK(std::string(itemText(ItemKind::ShotsGrand).label) == "grand amulet of shots");
	CHECK(std::string(itemText(ItemKind::QuickShotsLesser).label) == "lesser amulet of quick shots");
	CHECK(std::string(itemText(ItemKind::ShotsNormal).effect) == "Ranged weapons hold 75% more shots");
	CHECK(std::string(itemText(ItemKind::QuickShotsMinor).effect) == "Shots come back 50% faster");
}

TEST_CASE("each tier's shots for each ranged weapon: the share rounded up, at least one more") {
	struct Row {
		ItemKind weapon;
		int base;
		int byTier[AMULET_TIER_COUNT];
	};
	const Row rows[] = {
		{ItemKind::SelfBow, 5, {7, 8, 9, 10}},
		{ItemKind::CompositeBow, 6, {8, 9, 11, 12}},
		{ItemKind::Sling, 6, {8, 9, 11, 12}},
		{ItemKind::ThrowingStick, 3, {4, 5, 6, 6}},
		{ItemKind::Javelin, 2, {3, 3, 4, 4}},
	};
	PlayerStats stats;
	for (const Row& row : rows) {
		CAPTURE(itemText(row.weapon).label);
		stats.Wear(amuletBonus(std::nullopt), true);
		CHECK(stats.Shots(row.weapon) == row.base);
		for (size_t tier = 0; tier < AMULET_TIER_COUNT; tier++) {
			stats.Wear(amuletBonus(SHOTS[tier]), true);
			CHECK(stats.Shots(row.weapon) == row.byTier[tier]);
		}
		stats.Wear(amuletBonus(ItemKind::QuickShotsGrand), true);
		CHECK(stats.Shots(row.weapon) == row.base);
	}
	CHECK(stats.Shots(ItemKind::Club) == 0); // melee: no shots, amulet or not
	stats.Wear(amuletBonus(ItemKind::ShotsGrand), true);
	CHECK(stats.Shots(ItemKind::Club) == 0);
}

TEST_CASE("each tier's recovery: the wait after the last shot and the time per shot both shrink") {
	PlayerStats stats;
	CHECK(stats.ShotRecoveryPace().delayMs == SHOT_RECOVERY_DELAY_MS);
	CHECK(stats.ShotRecoveryPace().everyMs == SHOT_RECOVERY_MS);
	const int delay[AMULET_TIER_COUNT] = {1600, 1333, 1142, 1000};
	const int every[AMULET_TIER_COUNT] = {2000, 1666, 1428, 1250}; // the grand: a shot every 1.25 s, not 2.5
	for (size_t tier = 0; tier < AMULET_TIER_COUNT; tier++) {
		stats.Wear(amuletBonus(QUICK[tier]), true);
		CHECK(stats.ShotRecoveryPace().delayMs == delay[tier]);
		CHECK(stats.ShotRecoveryPace().everyMs == every[tier]);
	}
	stats.Wear(amuletBonus(ItemKind::ShotsGrand), true);
	CHECK(stats.ShotRecoveryPace().everyMs == SHOT_RECOVERY_MS);
}

namespace {
// tests/levels/archery (the plant at col 13, the player at col 1): the javelin in hand, facing away from the plant (the
// throws land on the floor), the amulet on.
void throwRange(SimWorld& world, ItemKind amulet) {
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/archery"));
	world.Equip(ItemKind::Javelin);
	world.Wear(amulet);
	world.WaitTicks(30);
	REQUIRE(world.Walk(3.f));
	REQUIRE(world.Walk(-0.2f));
}
int javelinsLeft(const SimWorld& world) {
	return world.items.ShotsLeft(ItemKind::Javelin, world.player.stats.Shots(ItemKind::Javelin));
}
} // namespace

TEST_CASE("in the world: the grand quiver throws four javelins, then none") {
	SimWorld world;
	throwRange(world, ItemKind::ShotsGrand);
	for (int i = 0; i < 4; i++) {
		REQUIRE(world.Attack());
		world.Wait(weaponDef(ItemKind::Javelin).motion.AttackMs());
	}
	CHECK(javelinsLeft(world) == 0);
	CHECK_FALSE(world.Attack());
}

TEST_CASE("in the world: with the grand cat a javelin is back after 1 + 1.25 s, not 2 + 2.5") {
	SimWorld world;
	throwRange(world, ItemKind::QuickShotsGrand);
	for (int i = 0; i < 2; i++) {
		REQUIRE(world.Attack());
		world.Wait(weaponDef(ItemKind::Javelin).motion.AttackMs());
	}
	CHECK(javelinsLeft(world) == 0);
	world.Wait((SHOT_RECOVERY_DELAY_MS + SHOT_RECOVERY_MS) / 2);
	CHECK(javelinsLeft(world) == 1);
	CHECK(world.Attack());
}
