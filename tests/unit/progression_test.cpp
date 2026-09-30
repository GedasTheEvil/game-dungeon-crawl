#include "../../external/doctest/doctest.h"
#include "../../src/world/progression.h"

TEST_CASE("level XP") {
	CHECK(levelXP(1) == 0.0);
	CHECK(levelXP(2) == doctest::Approx(1000.0));
	CHECK(levelXP(3) > 2 * levelXP(2)); // the gaps grow
}

TEST_CASE("progress through a level") {
	CHECK(levelProgress(1, 0) == 0.f);
	CHECK(levelProgress(1, 500) == doctest::Approx(0.5f));
	CHECK(levelProgress(2, levelXP(2)) == 0.f);
	CHECK(levelProgress(1, 5000) == 1.f); // clamped
}

TEST_CASE("a riddle is worth about a third of a level, at least 500") {
	CHECK(riddleXP(1) == 500); // 0.3 x 1000 = 300
	int gap = static_cast<int>(levelXP(11) - levelXP(10));
	CHECK(riddleXP(10) == static_cast<int>(gap * 0.3));
}
