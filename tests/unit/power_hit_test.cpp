// Power hit (plan skill-power-hit): with the skill and the stamina-skill toggle on, a melee attack held down charges,
// draining stamina up to a cap; the release swings with a bonus on the log curve of the points spent.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
// The fire level's mummy awake and next to the player, the sword in hand; with the skill learnt (skill) and the
// stamina-skill toggle (toggle).
void readyPowerHit(SimWorld& world, bool skill, bool toggle, ItemKind weapon = ItemKind::ShortSword) {
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/fire"));
	world.Equip(weapon);
	if (skill) {
		world.player.stats.heka.Earn(5);
		REQUIRE(world.player.stats.heka.Learn(SkillId::PowerHit));
	}
	world.player.staminaSkills = toggle;
	REQUIRE(world.WalkTo(12.f)); // the mummy wakes and climbs out
	REQUIRE(world.WalkTo(9.5f)); // back
	world.Wait(3000);
	const Monster* mummy = world.dungeon.NearestMonster();
	REQUIRE(mummy != nullptr);
	REQUIRE(world.WalkTo(mummy->Left() - world.player.HalfWidth() - 0.1f)); // up to it, facing it
	REQUIRE(world.Nearest() == monsterKind(MonsterMummy)->maxHealth);
}

// The attack key held for ms, then until the swing is over: the mummy's HP lost.
int heldDamage(SimWorld& world, int ms) {
	const int before = world.Nearest();
	REQUIRE(world.HoldAttack(ms));
	world.Wait(1500);
	return before - world.Nearest();
}
} // namespace

TEST_CASE("power hit: the bonus follows the log curve of the stamina points spent") {
	CHECK(powerHitBonusPercent(0) == 0);
	CHECK(powerHitBonusPercent(10) == 37);
	CHECK(powerHitBonusPercent(25) == 75); // the level 1 cap
	CHECK(powerHitBonusPercent(50) == 115);
	CHECK(powerHitBonusPercent(100) == 165);
	CHECK(powerHitBonusPercent(148) == 196); // the level 50 cap
}

TEST_CASE("power hit: the charge spends up to 25% of max stamina, more max spends more and hits harder") {
	SimWorld plain;
	readyPowerHit(plain, true, false);
	const int normal = heldDamage(plain, 2000); // toggle off: a plain swing at the press
	CHECK(normal > 0);
	CHECK(plain.player.stats.Stamina() == plain.player.stats.MaxStamina());

	SimWorld world;
	readyPowerHit(world, true, true);
	REQUIRE(world.player.stats.MaxStamina() == 100);
	REQUIRE(world.player.StartCharge());
	world.Wait(2000); // held well past the cap (~0.85 s)
	CHECK(world.player.Charging());
	CHECK(world.player.charge.spent == 25);
	CHECK(world.player.stats.Stamina() == 75); // no regen while charging
	world.player.ReleaseCharge();
	CHECK(world.player.swingPower == 25);
	CHECK(world.player.PowerDamage(100) == 175);

	SimWorld strong;
	readyPowerHit(strong, true, true);
	strong.player.stats.AddMaxStamina(100); // an inscription boon's: max 200
	REQUIRE(strong.player.stats.MaxStamina() == 200);
	const int charged = heldDamage(strong, 2000);
	CHECK(strong.player.swingPower == 50);
	// The bonus is on the weapon's damage, before the mummy's weakness to slash: a point of rounding apart.
	CHECK(std::abs(charged - normal * (100 + powerHitBonusPercent(50)) / 100) <= 1);

	SimWorld halfway;
	readyPowerHit(halfway, true, true);
	CHECK(heldDamage(halfway, 2000) > normal);
	CHECK(halfway.player.swingPower == 25);
}

TEST_CASE("power hit: the drain grows with the hold, at 30% of max stamina a second") {
	SimWorld world;
	readyPowerHit(world, true, true);
	REQUIRE(world.player.StartCharge());
	world.Wait(100);
	CHECK(world.player.charge.spent == 0); // a tap's time: nothing yet
	world.Wait(400); // 0.5 s: 15 points
	CHECK(world.player.charge.spent == 15);
	world.Wait(300); // 0.8 s: 24
	CHECK(world.player.charge.spent == 24);
	world.Wait(100); // the cap
	CHECK(world.player.charge.spent == 25);
}

TEST_CASE("power hit: a tap costs nothing and deals normal damage") {
	SimWorld plain;
	readyPowerHit(plain, true, false);
	const int normal = heldDamage(plain, 50);
	CHECK(normal > 0);

	SimWorld world;
	readyPowerHit(world, true, true);
	CHECK(heldDamage(world, 100) == normal);
	CHECK(world.player.swingPower == 0);
	CHECK(world.player.stats.Stamina() == world.player.stats.MaxStamina());
}

TEST_CASE("power hit: toggle off or no skill, holding does not charge; a bow draw never charges") {
	SimWorld off;
	readyPowerHit(off, true, false);
	CHECK_FALSE(off.player.CanCharge(false));
	SimWorld unskilled;
	readyPowerHit(unskilled, false, true);
	CHECK_FALSE(unskilled.player.CanCharge(false));

	SimWorld bow;
	readyPowerHit(bow, true, true, ItemKind::SelfBow);
	CHECK(bow.player.CanCharge(false));
	CHECK_FALSE(bow.player.CanCharge(true));
	// Back off from the mummy: closer it is too close to shoot (plan ranged-weaker-than-melee).
	REQUIRE(bow.WalkTo(bow.X() - 3.f));
	bow.facing = 1;
	REQUIRE(bow.dungeon.ShotRefused(ItemKind::SelfBow, 1) == Dungeon::ShotRefusal::None);
	REQUIRE(bow.HoldAttack(1000)); // a plain bow shot at the press
	CHECK_FALSE(bow.player.Charging());
	CHECK(bow.player.swingPower == 0);
	CHECK(bow.player.stats.Stamina() == bow.player.stats.MaxStamina());
}

TEST_CASE("power hit: stamina running out mid-charge swings with what was built") {
	SimWorld plain;
	readyPowerHit(plain, true, false);
	const int normal = heldDamage(plain, 50);

	SimWorld world;
	readyPowerHit(world, true, true);
	world.player.stats.SetStamina(10);
	const int damage = heldDamage(world, 2000);
	CHECK(world.player.swingPower == 10);
	CHECK(std::abs(damage - normal * (100 + powerHitBonusPercent(10)) / 100) <= 1);
}

TEST_CASE("power hit: too little stamina to start is a normal attack, free, and the sign blinks") {
	SimWorld world;
	readyPowerHit(world, true, true);
	world.player.stats.SetStamina(3); // under a tap's worth (4.5)
	CHECK_FALSE(world.player.StartCharge());
	CHECK(world.player.staminaSkillRefusedMs.has_value());
	CHECK_FALSE(world.player.Charging());
}

TEST_CASE("power hit: no stamina regen and no sprint while charging") {
	SimWorld world;
	readyPowerHit(world, true, true);
	world.player.stats.SetStamina(50);
	REQUIRE(world.player.StartCharge());
	world.Wait(100); // in the tap's time nothing drains, nothing comes back
	CHECK(world.player.stats.Stamina() == 50);
	world.player.stats.SetSprintRequested(true);
	CHECK(world.player.stats.SprintMoveMultiplier() == doctest::Approx(1.f));
	world.player.stats.SetSprintRequested(false);
	world.player.ReleaseCharge();
	world.Wait(3000);
	CHECK(world.player.stats.Stamina() > 50); // regenerates again
}
