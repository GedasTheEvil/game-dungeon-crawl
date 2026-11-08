// Mouse look (plan mouse-look-fixes): no turn behind a screen, no jump after one or after the cursor left the window.
#include "../../external/doctest/doctest.h"
#include "../../src/level/mouse_look.h"

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

// Plan mouse-leaves-window: the pointer held in the window by a warp to its centre.
TEST_CASE("mouse look: a pointer far off the centre warps back, the warp turns nothing, the look keeps turning") {
	MouseLook look;
	look.Motion(400, 300, true);
	CHECK(!look.Hold(800, 600)); // at the centre: no warp
	CHECK(look.Motion(560, 300, true).dx == 160);
	CHECK(!look.Hold(800, 600)); // within a quarter of the window
	CHECK(look.Motion(790, 300, true).dx == 230); // near the right edge
	const std::optional<MouseLook::Point> to = look.Hold(800, 600);
	REQUIRE(to);
	CHECK(to->x == 400);
	CHECK(to->y == 300);
	CHECK(still(look.Motion(400, 300, true))); // the warp's own motion
	int turned = 0;
	for (int i = 0; i < 20; i++) { // 5000 pixels to the right, six window widths: it keeps turning
		turned += look.Motion(650, 300, true).dx;
		REQUIRE(look.Hold(800, 600));
		CHECK(still(look.Motion(400, 300, true)));
	}
	CHECK(turned == 20 * 250);
}

TEST_CASE("mouse look: motions queued before the warp are dropped, a lost warp motion starts afresh") {
	MouseLook look;
	look.Motion(10, 10, true);
	look.Motion(5, 10, true);
	REQUIRE(look.Hold(800, 600));
	CHECK(still(look.Motion(3, 10, true))); // stale: would be a jump of -397
	CHECK(still(look.Motion(400, 300, true)));
	CHECK(look.Motion(395, 300, true).dx == -5);

	REQUIRE(look.Hold(800, 600) == std::nullopt); // near the centre
	look.Motion(700, 300, true);
	REQUIRE(look.Hold(800, 600));
	for (int i = 0; i < MOUSE_WARP_STALE; i++)
		CHECK(still(look.Motion(710 + i, 300, true)));
	CHECK(still(look.Motion(720, 300, true))); // the warp's motion never came: only sets the position
	CHECK(look.Motion(722, 300, true).dx == 2);
}

TEST_CASE("mouse look: a screen over the game keeps the free pointer where it is, coming back does not jump") {
	MouseLook look;
	look.Motion(700, 500, false); // the inventory: the caller does not ask Hold
	const MouseLook::Turn back = look.Motion(702, 500, true);
	CHECK(back.dx == 2);
	CHECK(look.Hold(800, 600)); // the first look motion back: now held
}
