#include "../../external/doctest/doctest.h"
#include "../../src/world/view_window.h"
#include <cmath>

namespace {
constexpr float TILE = 40.f;

// The game's camera (drawGameplay, Dungeon::Draw) with the player on a cell's left edge: gluPerspective(45, aspect)
// times the translations down to Dungeon::Draw's frame, column-major.
void gameClip(float aspect, float clip[16]) {
	const float f = 1.f / std::tan(22.5f * 3.14159265f / 180.f);
	const float zNear = 1.f;
	const float zFar = 2000.f;
	const float tx = -200.f + 2.f * TILE; // the player at x 0, then Dungeon::Draw's shift to the window's origin cell
	const float ty = -20.f - 120.f;
	const float tz = -70.f - 10.f;
	const float a = (zFar + zNear) / (zNear - zFar);
	const float b = 2.f * zFar * zNear / (zNear - zFar);
	for (int i = 0; i < 16; i++)
		clip[i] = 0.f;
	clip[0] = f / aspect;
	clip[5] = f;
	clip[10] = a;
	clip[11] = -1.f;
	clip[12] = f / aspect * tx;
	clip[13] = f * ty;
	clip[14] = a * tz + b;
	clip[15] = -tz;
}
} // namespace

TEST_CASE("the drawn window covers what the camera sees") {
	const ViewWindow view{20, 10}; // the player in column 23
	const CellRect base = view.cells();
	float clip[16];

	SUBCASE("16:9 sees less than the gameplay window: that is drawn") {
		gameClip(16.f / 9.f, clip);
		const CellRect drawn = drawnWindow(view, clip, TILE);
		CHECK(drawn.col0 == base.col0);
		CHECK(drawn.cols == base.cols);
		CHECK(drawn.row0 == base.row0);
		CHECK(drawn.rows == base.rows);
	}
	SUBCASE("a wide window draws more columns, not more rows") {
		gameClip(1920.f / 300.f, clip); // about 8 tiles each side at the back wall
		const CellRect drawn = drawnWindow(view, clip, TILE);
		CHECK(drawn.col0 <= 23 - 8);
		CHECK(drawn.col0 + drawn.cols >= 23 + 9);
		CHECK(drawn.rows == base.rows);
	}
	SUBCASE("a very wide window is capped") {
		gameClip(100.f, clip);
		const CellRect drawn = drawnWindow(view, clip, TILE);
		CHECK(drawn.col0 == base.col0 - DRAWN_EXTRA_COLS);
		CHECK(drawn.cols == base.cols + 2 * DRAWN_EXTRA_COLS);
	}
}
