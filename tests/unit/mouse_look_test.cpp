// Mouse look (plan mouse-look-fixes): no turn behind a screen, no jump after one or after the cursor left the window.
#include "../../external/doctest/doctest.h"
#include "../../src/input/mouse_look.h"

namespace {
bool still(MouseLook::Turn turn) { return turn.dx == 0 && turn.dy == 0; }
} // namespace

TEST_CASE("mouse look: the first motion only sets the position, the next ones turn by their delta") {
	MouseLook look;
	CHECK(still(look.Motion(400, 300, true)));
	const MouseLook::Turn turn = look.Motion(410, 295, true);
	CHECK(turn.dx == 10);
	CHECK(turn.dy == -5);
}

TEST_CASE("mouse look: a screen over the game (the riddle) turns nothing, and the game does not jump after it") {
	MouseLook look;
	look.Motion(100, 100, true);
	CHECK(still(look.Motion(500, 400, false)));
	const MouseLook::Turn back = look.Motion(503, 400, true);
	CHECK(back.dx == 3);
	CHECK(back.dy == 0);
}

TEST_CASE("mouse look: after the cursor left and entered the window, the first motion turns nothing") {
	MouseLook look;
	look.Motion(10, 10, true);
	look.Forget();
	CHECK(still(look.Motion(700, 500, true)));
	CHECK(look.Motion(701, 500, true).dx == 1);
}
