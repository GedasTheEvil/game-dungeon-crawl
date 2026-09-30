#include "../../external/doctest/doctest.h"
#include "../../src/world/movement.h"

TEST_CASE("the jump arc from the game's constants") {
	// 0.085 up, 0.01 less each 40 ms tick: 18 ticks, peak after 9.
	CHECK(Jump::ARC.ticks == 18);
	CHECK(Jump::ARC.ms() == 720);
	CHECK(Jump::ARC.peak == doctest::Approx(0.405f));
	CHECK(Jump::ARC.drift == doctest::Approx(0.972f));
}
