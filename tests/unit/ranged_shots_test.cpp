// Ranged weapons weaker than melee (plan ranged-weaker-than-melee): a few shots that come back one by one, no shot at
// a monster too close, and softer hits than the melee twin. The HUD's pips: tests/scenarios/solved/ranged_shots.txt.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include <algorithm>
#include <sstream>
#include <string>

namespace {
constexpr ShotPace PACE = {SHOT_RECOVERY_DELAY_MS, SHOT_RECOVERY_MS};

bool heard(const SimWorld& world, WorldSound sound) {
	return std::any_of(world.said.begin(), world.said.end(), [&](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Sound && e.sound == sound;
	});
}

// The bow, the arrow shot and its recovery over.
void shootAndRecover(SimWorld& world) {
	REQUIRE(world.Attack());
	world.Wait(weaponDef(world.items.Equipped()).motion.AttackMs());
}
} // namespace

TEST_CASE("shots spend, run out and come back one by one after the wait") {
	ItemBag bag;
	const int shots = weaponDef(ItemKind::SelfBow).shots;
	REQUIRE(shots == 5);
	CHECK(bag.ShotsLeft(ItemKind::SelfBow, shots) == shots);
	for (int i = 0; i < shots; i++)
		bag.SpendShot(ItemKind::SelfBow);
	CHECK(bag.ShotsLeft(ItemKind::SelfBow, shots) == 0);
	bag.RecoverShots(SHOT_RECOVERY_DELAY_MS, PACE);
	CHECK(bag.ShotRecovery(ItemKind::SelfBow, PACE) == doctest::Approx(0.f)); // the wait after the shot
	bag.RecoverShots(SHOT_RECOVERY_MS / 2, PACE);
	CHECK(bag.ShotRecovery(ItemKind::SelfBow, PACE) == doctest::Approx(0.5f));
	CHECK(bag.ShotsLeft(ItemKind::SelfBow, shots) == 0);
	bag.RecoverShots(SHOT_RECOVERY_MS / 2, PACE);
	CHECK(bag.ShotsLeft(ItemKind::SelfBow, shots) == 1);
	bag.RecoverShots(SHOT_RECOVERY_MS - 1, PACE);
	CHECK(bag.ShotsLeft(ItemKind::SelfBow, shots) == 1);
	bag.RecoverShots(1, PACE);
	CHECK(bag.ShotsLeft(ItemKind::SelfBow, shots) == 2);
	bag.RecoverShots(10 * SHOT_RECOVERY_MS, PACE);
	CHECK(bag.ShotsLeft(ItemKind::SelfBow, shots) == shots);
	CHECK(bag.ShotRecovery(ItemKind::SelfBow, PACE) == doctest::Approx(0.f)); // full
}

TEST_CASE("a shot restarts the wait before the shots come back") {
	ItemBag bag;
	const int shots = weaponDef(ItemKind::SelfBow).shots;
	bag.SpendShot(ItemKind::SelfBow);
	bag.RecoverShots(SHOT_RECOVERY_DELAY_MS + SHOT_RECOVERY_MS - 100, PACE);
	bag.SpendShot(ItemKind::SelfBow);
	bag.RecoverShots(SHOT_RECOVERY_DELAY_MS + SHOT_RECOVERY_MS - 1, PACE);
	CHECK(bag.ShotsLeft(ItemKind::SelfBow, shots) == shots - 2);
	bag.RecoverShots(1, PACE);
	CHECK(bag.ShotsLeft(ItemKind::SelfBow, shots) == shots - 1);
}

TEST_CASE("each weapon keeps its own shots: a switch does not refill, all of them come back at once") {
	ItemBag bag;
	bag.Add(ItemKind::SelfBow);
	bag.Add(ItemKind::Sling);
	const int bow = weaponDef(ItemKind::SelfBow).shots, sling = weaponDef(ItemKind::Sling).shots;
	REQUIRE(bag.Use(ItemKind::SelfBow, BagUser{true, 1, 1, 1, 1}));
	bag.SpendShot(ItemKind::SelfBow);
	bag.SpendShot(ItemKind::SelfBow);
	REQUIRE(bag.Use(ItemKind::Sling, BagUser{true, 1, 1, 1, 1}));
	bag.SpendShot(ItemKind::Sling);
	REQUIRE(bag.Use(ItemKind::SelfBow, BagUser{true, 1, 1, 1, 1}));
	CHECK(bag.ShotsLeft(ItemKind::SelfBow, bow) == bow - 2);
	CHECK(bag.ShotsLeft(ItemKind::Sling, sling) == sling - 1);
	bag.RecoverShots(SHOT_RECOVERY_DELAY_MS + SHOT_RECOVERY_MS, PACE); // the sling, not in hand, too
	CHECK(bag.ShotsLeft(ItemKind::SelfBow, bow) == bow - 1);
	CHECK(bag.ShotsLeft(ItemKind::Sling, sling) == sling);
}

TEST_CASE("a load starts with every shot back") {
	ItemBag bag;
	bag.Add(ItemKind::Javelin);
	bag.SpendShot(ItemKind::Javelin);
	std::stringstream file;
	bag.Save(file);
	bag.Load(file);
	CHECK(bag.ShotsLeft(ItemKind::Javelin, weaponDef(ItemKind::Javelin).shots) == 2);
}

TEST_CASE("with no shot left the attack draws nothing, a dry click; the recovered shot flies") {
	SimWorld world; // tests/levels/archery: the plant at col 13, the player at col 1
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/archery"));
	world.Equip(ItemKind::Javelin);
	world.WaitTicks(30);
	REQUIRE(world.Walk(3.f));
	REQUIRE(world.Walk(-0.2f)); // facing away from the plant: the throws land on the floor
	for (int i = 0; i < weaponDef(ItemKind::Javelin).shots; i++)
		shootAndRecover(world);
	CHECK(world.items.ShotsLeft(ItemKind::Javelin, 2) == 0);
	CHECK_FALSE(heard(world, WorldSound::ShotDry));
	CHECK_FALSE(world.Attack());
	CHECK(world.player.attackStartMs < 0);
	world.Tick();
	CHECK(heard(world, WorldSound::ShotDry));
	CHECK(world.player.shotRefusedMs.has_value());
	CHECK_FALSE(world.player.shotTooClose);
	world.Wait(SHOT_RECOVERY_DELAY_MS + SHOT_RECOVERY_MS);
	CHECK(world.items.ShotsLeft(ItemKind::Javelin, 2) == 1);
	CHECK(world.Attack());
}

TEST_CASE("a monster too close: no shot, nothing spent; from farther the shot flies") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/archery"));
	world.Equip(ItemKind::SelfBow);
	world.WaitTicks(30);
	REQUIRE(world.Walk(11.6f)); // the plant's near edge under the bow's 0.8 tiles ahead
	world.WaitTicks(10);
	const int plant = world.Nearest();
	CHECK(world.dungeon.ShotRefused(ItemKind::SelfBow, 1) == Dungeon::ShotRefusal::TooClose);
	CHECK_FALSE(world.Attack());
	world.Tick();
	CHECK(world.Told("Too close to shoot"));
	CHECK(heard(world, WorldSound::ShotTooClose));
	CHECK(world.player.shotTooClose);
	CHECK(world.items.ShotsLeft(ItemKind::SelfBow, 5) == 5);
	// Facing away from it, nothing is too close.
	CHECK(world.dungeon.ShotRefused(ItemKind::SelfBow, -1) == Dungeon::ShotRefusal::None);
	REQUIRE(world.Walk(-2.1f));
	REQUIRE(world.Walk(0.2f)); // facing it again, 2.6 tiles off
	world.WaitTicks(10);
	CHECK(world.Attack());
	world.Wait(1500);
	CHECK(world.items.ShotsLeft(ItemKind::SelfBow, 5) == 4);
	CHECK(world.Nearest() < plant);
}

TEST_CASE("the minimum range counts only monsters: a wall right ahead never blocks a shot") {
	SimWorld world; // the archery range's east end: the wall at col 23
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/archery"));
	world.Equip(ItemKind::SelfBow);
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(15.f)); // past the plant
	REQUIRE(world.WalkTo(22.6f)); // stopped at the wall
	CHECK(world.dungeon.ShotRefused(ItemKind::SelfBow, 1) == Dungeon::ShotRefusal::None);
	CHECK(world.Attack());
}

TEST_CASE("each ranged weapon hits softer than its melee twin, per hit and per second, at every weapon level") {
	struct Twin {
		ItemKind ranged, melee;
	};
	constexpr Twin TWINS[] = {
		{ItemKind::ThrowingStick, ItemKind::Club}, // the same stick, blunt
		{ItemKind::Sling, ItemKind::Club}, // blunt, from the start
		{ItemKind::Javelin, ItemKind::Spear}, // the same pole, pierce
		{ItemKind::SelfBow, ItemKind::Dagger}, // pierce, from the start
		{ItemKind::CompositeBow, ItemKind::DuckbillAxe}, // mostly pierce, from depth 7
	};
	int checked = 0;
	for (const Twin& t : TWINS) {
		CAPTURE(std::string(itemText(t.ranged).name));
		const WeaponDef& ranged = weaponDef(t.ranged);
		const WeaponDef& melee = weaponDef(t.melee);
		CHECK(isRanged(t.ranged));
		CHECK_FALSE(isRanged(t.melee));
		CHECK(ranged.growthPercent == melee.growthPercent); // the caps hold at every level
		int last = 0;
		for (int level = 1; level <= MAX_WEAPON_LEVEL; level++) {
			CAPTURE(level);
			const int hit = weaponDamage(t.ranged, ranged.damage, level);
			const int twinHit = weaponDamage(t.melee, melee.damage, level);
			CHECK(hit > last); // a level-up shows a gain
			CHECK(hit < twinHit);
			CHECK(hit * melee.motion.AttackMs() * 100 <= twinHit * ranged.motion.AttackMs() * RANGED_TWIN_DPS_PERCENT);
			last = hit;
		}
		checked++;
	}
	int rangedKinds = 0;
	for (int i = 0; i < WEAPON_KIND_COUNT; i++)
		rangedKinds += isRanged(itemAt(i)) && !isTestWeapon(itemAt(i)) ? 1 : 0; // a test weapon has no twin
	CHECK(checked == rangedKinds); // every ranged weapon has its twin here
}

TEST_CASE("with palsy the bow draws nothing and spends no shot; the sword still swings (plan player-control-in-sim)") {
	SimWorld world; // tests/levels/archery: the plant at col 13, the player at col 1
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/archery"));
	world.Equip(ItemKind::SelfBow);
	world.Wait(2000);
	REQUIRE(world.player.attackTimer.TimePassed(true));
	REQUIRE(world.player.stats.LayCurse(Curse::Palsy, CurseGrade::Lesser, world.events, testRng()));
	const int shots = weaponDef(ItemKind::SelfBow).shots;
	CHECK_FALSE(world.Attack());
	CHECK(world.player.attackStartMs < 0);
	world.Tick();
	CHECK(world.items.ShotsLeft(ItemKind::SelfBow, shots) == shots);
	CHECK(world.Told("Your hands shake too much to aim"));
	world.Equip(ItemKind::ShortSword);
	CHECK(world.Attack());
}

TEST_CASE("a refused shot costs no attack interval (plan player-control-in-sim)") {
	SimWorld world; // tests/levels/archery
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/archery"));
	world.Equip(ItemKind::Javelin);
	world.WaitTicks(30);
	REQUIRE(world.Walk(3.f));
	REQUIRE(world.Walk(-0.2f)); // facing away from the plant
	for (int i = 0; i < weaponDef(ItemKind::Javelin).shots; i++)
		shootAndRecover(world);
	REQUIRE(world.player.attackTimer.TimePassed(true));
	CHECK_FALSE(world.Attack()); // no shot left
	CHECK(world.player.attackTimer.TimePassed(true)); // the refusal did not restart the attack timer
}

// Eye of Horus (plan skill-eye-of-horus): rank I, too close only while the hitboxes overlap; rank II, never.
TEST_CASE("Eye of Horus: a monster near but not on the player is no longer too close") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/archery"));
	world.Equip(ItemKind::SelfBow);
	world.WaitTicks(30);
	REQUIRE(world.Walk(11.6f));
	world.WaitTicks(10);
	const Monster* plant = world.dungeon.NearestMonster();
	REQUIRE(plant != nullptr);
	const float gap = plant->Left() - (world.X() + world.player.HalfWidth());
	REQUIRE(gap > 0.f); // apart, under the bow's minimum range
	CHECK(world.dungeon.ShotRefused(ItemKind::SelfBow, 1) == Dungeon::ShotRefusal::TooClose);
	world.player.stats.heka.Earn(5);
	REQUIRE(world.player.stats.heka.Learn(SkillId::EyeOfHorus, true));
	CHECK(world.dungeon.ShotRefused(ItemKind::SelfBow, 1) == Dungeon::ShotRefusal::None);
	CHECK(weaponMinRange(ItemKind::SelfBow, world.player.stats.heka) == 0.f); // the sim's kite stops backing off
	CHECK(world.Attack());
}

TEST_CASE("Eye of Horus: hitboxes overlapping are too close at rank I, never at rank II") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/archery"));
	world.Equip(ItemKind::SelfBow);
	world.WaitTicks(30);
	world.player.stats.heka.Earn(5);
	world.player.stats.heka.Earn(10);
	REQUIRE(world.player.stats.heka.Learn(SkillId::EyeOfHorus, true));
	REQUIRE(world.Walk(11.6f));
	world.WaitTicks(10);
	const Monster* plant = nullptr;
	// Into the plant step by step until the boxes overlap.
	for (int i = 0; i < 400; i++) {
		plant = world.dungeon.NearestMonster();
		REQUIRE(plant != nullptr);
		if (plant->Left() < world.X() + world.player.HalfWidth())
			break;
		world.HoldWalk(1, 20);
	}
	REQUIRE(plant->Left() < world.X() + world.player.HalfWidth()); // the plant does not stop the walk
	CHECK(world.dungeon.ShotRefused(ItemKind::SelfBow, 1) == Dungeon::ShotRefusal::TooClose);
	REQUIRE(world.player.stats.heka.Upgrade(SkillId::EyeOfHorus));
	CHECK(world.dungeon.ShotRefused(ItemKind::SelfBow, 1) == Dungeon::ShotRefusal::None);
}
