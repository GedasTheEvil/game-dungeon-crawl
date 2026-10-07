#include "../../external/doctest/doctest.h"
#include "../../src/world/decor.h"
#include "../../src/world/level_gen.h"

TEST_CASE("decoration tiers by depth: cave, worked tunnel, tomb, temple") {
	CHECK(decorTier(1) == 0);
	CHECK(decorTier(3) == 0);
	CHECK(decorTier(4) == 1);
	CHECK(decorTier(7) == 1);
	CHECK(decorTier(8) == 2);
	CHECK(decorTier(12) == 2);
	CHECK(decorTier(13) == 3);
	CHECK(decorTier(30) == 3);
	CHECK(decorTier(DECOR_DEPTH_ALL) == DECOR_TIER_COUNT - 1);
}

TEST_CASE("a generated level's difficulty stands for a campaign depth") {
	CHECK(genDecorDepth(1) == 1);
	CHECK(decorTier(genDecorDepth(3)) == 1);
	CHECK(decorTier(genDecorDepth(10)) == DECOR_TIER_COUNT - 1);
}

TEST_CASE("every scattered prop and decal has a tier, the first one a cave") {
	for (int d = 0; d < DECOR_SCATTERED; d++) {
		CHECK(DECOR_TIERS[d] >= 0);
		CHECK(DECOR_TIERS[d] < DECOR_TIER_COUNT);
	}
	CHECK(DECOR_TIERS[DECOR_COFFIN] == -1); // placed with its mummy, at any depth
	for (int v = 1; v < DECOR_THOTH_VARIANTS; v++)
		CHECK(DECOR_TIERS[DECOR_THOTH + v] == DECOR_TIERS[DECOR_THOTH]);
	for (int t : DECAL_TIERS) {
		CHECK(t >= 0);
		CHECK(t < DECOR_TIER_COUNT);
	}
	CHECK(ROUGH_PERCENT[0] == 100); // the first levels are all cave
	CHECK(PAINTED_PERCENT[0] == 0);
}
