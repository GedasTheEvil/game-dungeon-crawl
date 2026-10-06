#include "../../external/doctest/doctest.h"
#include "../../src/world/damage.h"

TEST_CASE("without resistances a typed hit deals what it did untyped") {
	for (int dmg : {1, 2, 9, 26, 110, 1000})
		for (DamageMix mix : {DamageMix{60, 40, 0}, {0, 20, 80}, {100, 0, 0}, {50, 0, 50}}) {
			CHECK(playerHitDamage(dmg, mix, NO_RESISTANCES, 0, false) == dmg);
			CHECK(playerHitDamage(dmg, mix, NO_RESISTANCES, 5, false) == std::max(1, dmg - 5));
			CHECK(playerHitDamage(dmg, mix, NO_RESISTANCES, 5, true) == dmg);
		}
}

TEST_CASE("the resistance scales the raw hit before the armour") {
	const Resistances pierceHalf = {NORMAL, NORMAL, RESISTS};
	// Bat bite 10, 80 % pierce: 10 * (20 + 80 * 0.5) / 100 = 6, armour 2 then leaves 4.
	CHECK(playerHitDamage(10, {0, 20, 80}, pierceHalf, 2, false) == 4);
	// A blunt hit does not care about a pierce resistance.
	CHECK(playerHitDamage(20, {100, 0, 0}, pierceHalf, 2, false) == 18);
	// Still at least 1.
	CHECK(playerHitDamage(2, {0, 0, 100}, pierceHalf, 5, false) == 1);
}

TEST_CASE("the journal's sentence for each monster group's mix") {
	CHECK(attackSentence({60, 40, 0}) == "It hits me bluntly hard, with a hint of cutting pain.");
	CHECK(attackSentence({40, 60, 0}) == "It cuts me deep, with a hint of crushing pain.");
	CHECK(attackSentence({0, 30, 70}) == "It pierces me to the bone, with a hint of cutting pain.");
	CHECK(attackSentence({100, 0, 0}) == "It hits me bluntly hard.");
	CHECK(attackSentence({80, 0, 20}) == "It hits me bluntly hard, with a hint of piercing pain.");
	CHECK(attackSentence({50, 0, 50}) == "It crushes and pierces me alike.");
	CHECK(attackSentence({40, 0, 60}) == "It pierces me to the bone, with a hint of crushing pain.");
	CHECK(attackSentence({20, 30, 50}) == "It pierces me to the bone, with a hint of crushing pain and cutting pain.");
}
