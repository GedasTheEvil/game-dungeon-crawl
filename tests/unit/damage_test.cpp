#include "../../external/doctest/doctest.h"
#include "../../src/world/damage.h"
#include <cmath>

TEST_CASE("without resistances a typed hit deals what it did untyped") {
	for (int dmg : {1, 2, 9, 26, 110, 1000})
		for (DamageMix mix : {DamageMix{60, 40, 0}, {0, 20, 80}, {100, 0, 0}, {50, 0, 50}}) {
			CHECK(playerHitDamage(dmg, mix, NO_RESISTANCES, 0, false) == dmg);
			CHECK(
				playerHitDamage(dmg, mix, NO_RESISTANCES, 5, false) ==
				std::max(1, static_cast<int>(std::lround(dmg / 1.1)))
			);
			CHECK(playerHitDamage(dmg, mix, NO_RESISTANCES, 5, true) == dmg);
		}
}

TEST_CASE("armour takes a share of a hit: half at ARMOR_HALVES, never all, so a weak monster still hurts") {
	const DamageMix blunt = {100, 0, 0};
	CHECK(ARMOR_HALVES == 50);
	CHECK(playerHitDamage(100, blunt, NO_RESISTANCES, 10, false) == 83);
	CHECK(playerHitDamage(100, blunt, NO_RESISTANCES, 50, false) == 50);
	CHECK(playerHitDamage(26, blunt, NO_RESISTANCES, 32, false) == 16); // was 1 with a flat armour (the playthrough)
	CHECK(playerHitDamage(100, blunt, NO_RESISTANCES, 1000, false) == 5);
	CHECK(playerHitDamage(100, blunt, NO_RESISTANCES, -3, false) == 100); // a curse below 0 adds nothing
}

TEST_CASE("the resistance scales the raw hit before the armour") {
	const Resistances pierceHalf = {NORMAL, NORMAL, RESISTS, NORMAL};
	// Bat bite 10, 80 % pierce: 10 * (20 + 80 * 0.5) / 100 = 6, armour 10 then leaves 6 * 50 / 60 = 5.
	CHECK(playerHitDamage(10, {0, 20, 80}, pierceHalf, 10, false) == 5);
	// A blunt hit does not care about a pierce resistance.
	CHECK(playerHitDamage(20, {100, 0, 0}, pierceHalf, 10, false) == 17);
	// Still at least 1.
	CHECK(playerHitDamage(2, {0, 0, 100}, pierceHalf, 5, false) == 1);
}

TEST_CASE("the journal's sentence for each monster group's mix") {
	CHECK(attackSentence({60, 40, 0}) == "It smashes me hard, with a hint of cutting pain.");
	CHECK(attackSentence({40, 60, 0}) == "It cuts me deep, with a hint of smashing pain.");
	CHECK(attackSentence({0, 30, 70}) == "It stabs me to the bone, with a hint of cutting pain.");
	CHECK(attackSentence({100, 0, 0}) == "It smashes me hard.");
	CHECK(attackSentence({80, 0, 20}) == "It smashes me hard, with a hint of stabbing pain.");
	CHECK(attackSentence({50, 0, 50}) == "It smashes and stabs me alike.");
	CHECK(attackSentence({40, 0, 60}) == "It stabs me to the bone, with a hint of smashing pain.");
	CHECK(attackSentence({20, 30, 50}) == "It stabs me to the bone, with a hint of smashing pain and cutting pain.");
}

TEST_CASE("fire: a share like the others; armour does not stop it, only the fire guard cuts it") {
	const DamageMix fire = {0, 0, 0, 100};
	CHECK(resistedDamage(10, fire, NO_RESISTANCES) == 10);
	CHECK(resistedDamage(10, fire, {NORMAL, NORMAL, NORMAL, WEAK}) == 20);
	// A fire arrow (40 pierce, 60 fire) on a mummy: 10 * (40 * 25 + 60 * 200) / 10000 = 13.
	CHECK(resistedDamage(10, {0, 0, 40, 60}, {RESISTS, WEAK, TOUGH, WEAK}) == 13);
	for (int armor : {0, 3, 12, 40})
		CHECK(playerHitDamage(20, fire, NO_RESISTANCES, armor, false) == 20);
	CHECK(playerHitDamage(20, fire, {NORMAL, NORMAL, NORMAL, 20}, 12, false) == 4); // grand fire guard
	// A mixed hit: the armour takes its share of the blunt share only.
	CHECK(playerHitDamage(20, {40, 0, 0, 60}, NO_RESISTANCES, 50, false) == 16); // 12 fire + 8 / 2
	CHECK(attackSentence({40, 0, 0, 60}) == "It sets me on fire, with a hint of smashing pain.");
}

namespace {
int burnFor(Burning& b, int ms) {
	int hp = 0;
	for (int t = 0; t < ms; t += 16)
		hp += b.Advance(16);
	return hp;
}
} // namespace

TEST_CASE("burning: a fifth of the fire share a second for 3 s; a stronger fire takes over; doused, it stops") {
	Burning b;
	b.Ignite(0.0); // no fire, no burning
	CHECK_FALSE(b.Any());
	b.Ignite(10.0);
	CHECK(b.Any());
	CHECK(burnFor(b, 4000) == 6); // 2 a second
	CHECK_FALSE(b.Any());
	b.Ignite(2.0); // 0.4 a second: the fractions add up
	CHECK(burnFor(b, 4000) == 1);
	b.Ignite(10.0);
	CHECK(burnFor(b, 1000) == 2);
	b.Ignite(5.0); // weaker: the time starts over at the same rate
	CHECK(burnFor(b, 4000) == 6);
	b.Ignite(10.0);
	b.Douse();
	CHECK(burnFor(b, 4000) == 0);
}

TEST_CASE("fire in water: a tenth of the fire share, the rest unchanged (plan fire-in-water)") {
	CHECK(inWaterResist(NO_RESISTANCES) == Resistances{NORMAL, NORMAL, NORMAL, 10});
	CHECK(inWaterResist({RESISTS, WEAK, TOUGH, WEAK}) == Resistances{RESISTS, WEAK, TOUGH, 20});
	const DamageMix anubis = {0, 0, 0, 100};
	CHECK(playerHitDamage(100, anubis, inWaterResist(NO_RESISTANCES), 0, false) == 10);
	CHECK(playerHitDamage(100, anubis, NO_RESISTANCES, 0, false) == 100);
	// Mixed 60 fire : 40 blunt: only the fire share drops (60 -> 6), blunt 40 stays.
	const DamageMix mixed = {40, 0, 0, 60};
	CHECK(playerHitDamage(100, mixed, inWaterResist(NO_RESISTANCES), 0, false) == 46);
	CHECK(resistedDamage(100, mixed, inWaterResist(NO_RESISTANCES)) == 46);
	CHECK(fireDamage(100, mixed, inWaterResist(NO_RESISTANCES)) == doctest::Approx(6.0));
}
