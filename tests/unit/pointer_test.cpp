// The screens' shared click rules (plan shared-click-handling): a press fires on release over the same target, with
// the same mouse button.
#include "../../external/doctest/doctest.h"
#include "../../src/ui/pointer.h"

namespace {
constexpr int NONE = -1;
constexpr int LEFT = 0;
constexpr int RIGHT = 2;
} // namespace

TEST_CASE("pointer: a release over the target it went down on fires it") {
	ui::Pointer<int> p{NONE};
	p.Move(3);
	p.Down(3, LEFT);
	CHECK(p.Held(3));
	CHECK(p.Up(3, LEFT) == 3);
	CHECK_FALSE(p.Pressing());
	CHECK(p.hovered == 3);
}

TEST_CASE("pointer: a release over another target fires nothing") {
	ui::Pointer<int> p{NONE};
	p.Move(3);
	p.Down(3, LEFT);
	p.Move(4);
	CHECK_FALSE(p.Held(3));
	CHECK_FALSE(p.Held(4));
	CHECK(p.Up(4, LEFT) == NONE);
	CHECK_FALSE(p.Pressing());
}

TEST_CASE("pointer: a press on nothing fires nothing") {
	ui::Pointer<int> p{NONE};
	p.Down(NONE, LEFT);
	CHECK_FALSE(p.Pressing());
	CHECK_FALSE(p.Held(NONE));
	CHECK(p.Up(NONE, LEFT) == NONE);
}

TEST_CASE("pointer: Clear drops a held press") {
	ui::Pointer<int> p{NONE};
	p.Move(1);
	p.Down(1, LEFT);
	p.Clear();
	CHECK(p.hovered == NONE);
	CHECK_FALSE(p.Pressing());
	p.Move(1);
	CHECK(p.Up(1, LEFT) == NONE);
}

TEST_CASE("pointer: the right button fires on its own release only") {
	ui::Pointer<int> p{NONE};
	p.Move(5);
	p.Down(5, RIGHT);
	CHECK(p.Up(5, LEFT) == NONE); // another button: the press stays
	CHECK(p.Pressing());
	CHECK(p.Up(5, RIGHT) == 5);
	CHECK(p.button == RIGHT);
}

TEST_CASE("pointer: a struct target, none by default") {
	struct Hit {
		int kind = 0;
		int index = 0;
		bool operator==(const Hit&) const = default;
	};
	ui::Pointer<Hit> p;
	p.Move({1, 2});
	p.Down({1, 2}, LEFT);
	CHECK(p.Held({1, 2}));
	CHECK_FALSE(p.Held({1, 3}));
	CHECK(p.Up({1, 2}, LEFT) == Hit{1, 2});
}
