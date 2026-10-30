// The menu keys (plan keyboard-menus): which key moves or activates, and where a direction moves the focus.
#include "../../external/doctest/doctest.h"
#include "../../src/input/bindings.h"
#include "../../src/input/input.h"
#include "../../src/ui/ui_layout.h"
#include <vector>

using ui::Rect;

TEST_CASE("menu keys: the arrows, Enter and Space, whatever the bindings") {
	Bindings none;
	for (int action = 0; action < BIND_ACTION_COUNT; action++)
		for (int slot = 0; slot < BINDING_SLOTS; slot++)
			none.Clear(static_cast<BindAction>(action), slot);
	CHECK(navKeyOf(InputKey::Special(SPECIAL_MOVE_UP), none) == NavKey::Up);
	CHECK(navKeyOf(InputKey::Special(SPECIAL_MOVE_DOWN), none) == NavKey::Down);
	CHECK(navKeyOf(InputKey::Special(SPECIAL_MOVE_LEFT), none) == NavKey::Left);
	CHECK(navKeyOf(InputKey::Special(SPECIAL_MOVE_RIGHT), none) == NavKey::Right);
	CHECK(navKeyOf(InputKey::Char(KEY_ENTER), none) == NavKey::Activate);
	CHECK(navKeyOf(InputKey::Char(KEY_SPACE), none) == NavKey::Activate);
	CHECK(navKeyOf(InputKey::Char('w'), none) == NavKey::None);
}

TEST_CASE("menu keys: the bound move, climb and interact keys") {
	Bindings keys; // the defaults: WASD, E
	CHECK(navKeyOf(InputKey::Char('w'), keys) == NavKey::Up);
	CHECK(navKeyOf(InputKey::Char('S'), keys) == NavKey::Down);
	CHECK(navKeyOf(InputKey::Char('a'), keys) == NavKey::Left);
	CHECK(navKeyOf(InputKey::Char('d'), keys) == NavKey::Right);
	CHECK(navKeyOf(InputKey::Char('e'), keys) == NavKey::Activate);
	CHECK(navKeyOf(InputKey::Char('i'), keys) == NavKey::None);

	keys.Bind(BindAction::MoveLeft, 0, InputKey::Char('j'));
	CHECK(navKeyOf(InputKey::Char('j'), keys) == NavKey::Left);
	CHECK(navKeyOf(InputKey::Char('a'), keys) == NavKey::None);
}

TEST_CASE("menu focus: a column, a grid and a row under it") {
	// Two columns of two, a wide button under them (the Save Game screen's shape).
	const std::vector<Rect> rects = {
		{10, 50, 20, 8}, // 0 top left
		{10, 40, 20, 8}, // 1 bottom left
		{40, 50, 20, 8}, // 2 top right
		{40, 40, 20, 8}, // 3 bottom right
		{25, 20, 20, 8}, // 4 under both
	};
	CHECK(ui::nearestRect(rects, 0, 0, -1) == 1);
	CHECK(ui::nearestRect(rects, 1, 0, 1) == 0);
	CHECK(ui::nearestRect(rects, 0, 1, 0) == 2);
	CHECK(ui::nearestRect(rects, 3, -1, 0) == 1);
	CHECK(ui::nearestRect(rects, 1, 0, -1) == 4);
	CHECK(ui::nearestRect(rects, 3, 0, -1) == 4);
	CHECK(ui::nearestRect(rects, 0, 0, 1) == -1); // nothing above
	CHECK(ui::nearestRect(rects, 2, 1, 0) == -1);
	CHECK(ui::nearestRect(rects, -1, 0, 1) == -1);
}

TEST_CASE("menu focus: the next row's narrower control, not the one after it") {
	// The Options rows: a choice 30 wide, a switch 22 wide at the same right edge, another choice.
	const std::vector<Rect> rects = {{100, 70, 30, 4}, {108, 64, 22, 4}, {100, 58, 30, 4}};
	CHECK(ui::nearestRect(rects, 0, 0, -1) == 1);
	CHECK(ui::nearestRect(rects, 1, 0, -1) == 2);
	CHECK(ui::nearestRect(rects, 2, 0, 1) == 1);
}
