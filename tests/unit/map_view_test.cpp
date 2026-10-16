// The level editor's map view (plan editor-zoom).
#include "../../external/doctest/doctest.h"
#include "../../tools/editor/map_view.h"

TEST_CASE("map view: 1x shows the whole map, as before the zoom") {
	MapView view;
	CHECK(view.cellAt(EDITOR_MAP_X + 0.1f, EDITOR_MAP_Y + 0.1f).col == 0);
	CellPos top = view.cellAt(EDITOR_MAP_X + EDITOR_MAP_AREA.w - 0.1f, EDITOR_MAP_Y + EDITOR_MAP_AREA.h - 0.1f);
	CHECK(top.col == LEVEL_WIDTH - 1);
	CHECK(top.row == LEVEL_HEIGHT - 1);
	CHECK(view.cellAt(0.f, 0.f).col == -1);
	view.pan(5.f, 5.f); // nothing to pan
	CHECK(view.originCol() == 0.f);
	CHECK(view.originRow() == 0.f);
}

TEST_CASE("map view: cell rect and cell lookup agree at every zoom and offset") {
	MapView view;
	for (int zoom = 1; zoom <= 3; zoom++) {
		view.setZoom(zoom, 20.f, 23.f, 0.5f, 0.5f);
		view.pan(1.5f, -2.f);
		for (int col = 0; col < LEVEL_WIDTH; col++)
			for (int row = 0; row < LEVEL_HEIGHT; row++) {
				ui::Rect r = view.cellRect(col, row);
				if (!r.within(EDITOR_MAP_AREA.inset(-0.01f)))
					continue; // outside the window
				CellPos cell = view.cellAt(r.cx(), r.cy());
				CHECK(cell.col == col);
				CHECK(cell.row == row);
			}
	}
}

TEST_CASE("map view: zoom keeps the cell under the mouse, the window stays inside the map") {
	MapView view;
	view.setZoom(3, 10.f, 12.f, 0.25f, 0.75f);
	ui::Rect r = view.cellRect(10, 12);
	CHECK(r.x == doctest::Approx(EDITOR_MAP_X + 0.25f * EDITOR_MAP_AREA.w).epsilon(0.01));
	view.setZoom(3, 0.f, 0.f, 1.f, 1.f); // would start before the corner
	CHECK(view.originCol() == 0.f);
	CHECK(view.originRow() == 0.f);
	view.pan(100.f, 100.f);
	CHECK(view.originCol() == doctest::Approx(static_cast<float>(LEVEL_WIDTH) - view.visibleCols()));
	CHECK(view.originRow() == doctest::Approx(static_cast<float>(LEVEL_HEIGHT) - view.visibleRows()));
	view.setZoom(9, 5.f, 5.f, 0.5f, 0.5f);
	CHECK(view.zoom() == 3);
	view.setZoom(1, 5.f, 5.f, 0.5f, 0.5f);
	CHECK(view.originCol() == 0.f);
}

TEST_CASE("map view: centerOn clamps to the edges") {
	MapView view;
	view.setZoom(2, 20.f, 23.f, 0.5f, 0.5f);
	view.centerOn(0.f, 0.f);
	CHECK(view.originCol() == 0.f);
	view.centerOn(20.f, 23.5f);
	CHECK(view.originCol() == doctest::Approx(10.f));
}
