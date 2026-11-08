// Camera peek (plan controls-smoothing): the look keys turn the camera at a steady rate while held, an idle peek eases
// back to straight ahead.
#include "../../external/doctest/doctest.h"
#include "../../src/level/camera_look.h"

namespace {
using Dir = CameraLook::Dir;
using Angles = CameraLook::Angles;

Angles run(CameraLook& look, Angles a, int ticks, bool recenter = true) {
	for (int i = 0; i < ticks; i++)
		a = look.Step(a, UPDATE_TICK_MS, recenter);
	return a;
}

constexpr float TICK_TURN = CAMERA_LOOK_KEY_SPEED * static_cast<float>(UPDATE_TICK_MS) / 1000.f;
} // namespace

TEST_CASE("camera look: a look key held N ticks turns N tick steps, the release stops it") {
	CameraLook look;
	look.Hold(Dir::Right, true);
	Angles a = run(look, {}, 10);
	CHECK(a.m == doctest::Approx(10 * TICK_TURN));
	CHECK(a.n == 0.f);
	look.Hold(Dir::Right, false);
	CHECK(run(look, a, 10).m == doctest::Approx(a.m)); // within the recentre delay: stays

	look.Hold(Dir::Down, true);
	a = run(look, {}, 5);
	CHECK(a.n == doctest::Approx(-5 * TICK_TURN));
}

TEST_CASE("camera look: the turn speed does not depend on the key repeat, a quarter second turns a quarter speed") {
	CameraLook look;
	look.Hold(Dir::Left, true);
	look.Hold(Dir::Left, true); // a repeat: nothing more
	const Angles a = run(look, {}, 250 / UPDATE_TICK_MS);
	CHECK(a.m == doctest::Approx(-TICK_TURN * (250 / UPDATE_TICK_MS)));
	CHECK(a.m == doctest::Approx(-CAMERA_LOOK_KEY_SPEED / 4).epsilon(0.05));
}

TEST_CASE("camera look: the clamp holds") {
	CameraLook look;
	look.Hold(Dir::Up, true);
	look.Hold(Dir::Right, true);
	const Angles a = run(look, {}, 500);
	CHECK(a.m == CAMERA_ROTATE_LIMIT_X);
	CHECK(a.n == CAMERA_ROTATE_LIMIT_Y);
	const Angles c = CameraLook::Clamp({-100.f, -100.f});
	CHECK(c.m == -CAMERA_ROTATE_LIMIT_X);
	CHECK(c.n == -CAMERA_ROTATE_LIMIT_Y);
}

TEST_CASE("camera look: an idle peek waits, then eases back to 0 in about half a second") {
	CameraLook look;
	const Angles start{20.f, -8.f};
	const int delayTicks = CAMERA_RECENTER_DELAY_MS / UPDATE_TICK_MS;
	Angles a = run(look, start, delayTicks - 1);
	CHECK(a.m == start.m); // still waiting
	CHECK(a.n == start.n);
	a = run(look, a, 2);
	CHECK(std::abs(a.m) < start.m); // easing
	CHECK(a.m > 0.f);
	a = run(look, a, 500 / UPDATE_TICK_MS);
	CHECK(std::abs(a.m) < 0.06f * start.m); // settled within about 0.5 s
	a = run(look, a, 100);
	CHECK(a.m == 0.f);
	CHECK(a.n == 0.f);
}

TEST_CASE("camera look: any look input stops the easing at once and the wait starts again") {
	CameraLook look;
	Angles a = run(look, {20.f, 0.f}, CAMERA_RECENTER_DELAY_MS / UPDATE_TICK_MS + 5);
	REQUIRE(a.m < 20.f);
	look.Looked(); // the mouse turned it
	const Angles held = run(look, a, 10);
	CHECK(held.m == a.m);

	look.Hold(Dir::Left, true); // a look key held counts too
	a = run(look, {20.f, 0.f}, 1);
	look.Hold(Dir::Left, false);
	CHECK(run(look, a, 10).m == a.m);
}

TEST_CASE("camera look: with recentring off the peek stays") {
	CameraLook look;
	const Angles a = run(look, {15.f, 5.f}, 1000, false);
	CHECK(a.m == 15.f);
	CHECK(a.n == 5.f);
}

// Plan camera-recentre-undoes-mouse-look: the mouse is where the player looks, the recentre is for a look-key peek.
TEST_CASE("camera look: a mouse turn stays after a long idle, a look-key peek still eases back") {
	CameraLook look;
	look.Looked(); // the mouse turned it to here
	Angles a = run(look, {20.f, -5.f}, 3 * CAMERA_RECENTER_DELAY_MS / UPDATE_TICK_MS);
	CHECK(a.m == 20.f);
	CHECK(a.n == -5.f);

	look.Hold(Dir::Left, true); // a look-key peek: the recentre is back
	a = run(look, a, 1);
	look.Hold(Dir::Left, false);
	a = run(look, a, 2 * CAMERA_RECENTER_DELAY_MS / UPDATE_TICK_MS + 100);
	CHECK(a.m == 0.f);
	CHECK(a.n == 0.f);
}
