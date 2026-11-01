// The draft map's zoom and pan (plan map-symbols-and-zoom).
#include "../../external/doctest/doctest.h"
#include "../../src/ui/map_zoom.h"

namespace {
// A sheet 60 x 70 showing 30 x 35 cells at 1x, the cell (15, 17.5) at its centre.
constexpr MapFit FIT = {{10.f, 5.f, 60.f, 70.f}, 2.f, 15.f, 17.5f};

// Where a map point (in cells) lands on the canvas.
float canvasX(const MapFrame& f, float i) { return f.originX + i * f.cell; }
float canvasY(const MapFrame& f, float j) { return f.originY + j * f.cell; }
} // namespace

TEST_CASE("map zoom: 1x is the fit, nothing to pan") {
	MapZoom zoom;
	MapFrame f = zoom.Frame(FIT);
	CHECK(f.cell == doctest::Approx(2.f));
	CHECK(canvasX(f, 15.f) == doctest::Approx(FIT.area.cx()));
	CHECK(canvasY(f, 17.5f) == doctest::Approx(FIT.area.cy()));
	zoom.Pan(FIT, 20.f, -20.f);
	f = zoom.Frame(FIT);
	CHECK(canvasX(f, 15.f) == doctest::Approx(FIT.area.cx()));
	CHECK(canvasY(f, 17.5f) == doctest::Approx(FIT.area.cy()));
	zoom.ZoomAt(FIT, 0.5f, 20.f, 20.f); // no zooming out past the fit
	CHECK(zoom.Zoom(FIT) == doctest::Approx(1.f));
}

TEST_CASE("map zoom: zooming keeps the point under the mouse") {
	MapZoom zoom;
	const float x = 30.f, y = 50.f;
	const MapFrame before = zoom.Frame(FIT);
	const float i = (x - before.originX) / before.cell;
	const float j = (y - before.originY) / before.cell;
	zoom.ZoomAt(FIT, MapZoom::STEP, x, y);
	const MapFrame after = zoom.Frame(FIT);
	CHECK(after.cell == doctest::Approx(2.f * MapZoom::STEP));
	CHECK(canvasX(after, i) == doctest::Approx(x));
	CHECK(canvasY(after, j) == doctest::Approx(y));
}

TEST_CASE("map zoom: the view stays within the 1x view, the zoom within MAX_CELL") {
	MapZoom zoom;
	for (int k = 0; k < 40; k++)
		zoom.ZoomAt(FIT, MapZoom::STEP, FIT.area.cx(), FIT.area.cy());
	MapFrame f = zoom.Frame(FIT);
	CHECK(f.cell == doctest::Approx(MapZoom::MAX_CELL));
	zoom.Pan(FIT, 1000.f, 1000.f); // dragged far to the top right: the bottom left corner of the fit shows
	f = zoom.Frame(FIT);
	CHECK(canvasX(f, 0.f) == doctest::Approx(FIT.area.x));
	CHECK(canvasY(f, 0.f) == doctest::Approx(FIT.area.y));
	zoom.Pan(FIT, -5000.f, -5000.f);
	f = zoom.Frame(FIT);
	CHECK(canvasX(f, 30.f) == doctest::Approx(FIT.area.x + FIT.area.w));
	CHECK(canvasY(f, 35.f) == doctest::Approx(FIT.area.y + FIT.area.h));
	// Zoomed out again, it is the fit.
	for (int k = 0; k < 40; k++)
		zoom.ZoomAt(FIT, 1.f / MapZoom::STEP, FIT.area.x, FIT.area.y);
	f = zoom.Frame(FIT);
	CHECK(canvasX(f, 15.f) == doctest::Approx(FIT.area.cx()));
	zoom.Reset();
	CHECK(zoom.Zoom(FIT) == doctest::Approx(1.f));
}

TEST_CASE("map zoom: a fit already at MAX_CELL does not zoom") {
	constexpr MapFit BIG = {{0.f, 0.f, 60.f, 70.f}, MapZoom::MAX_CELL, 2.f, 2.5f};
	MapZoom zoom;
	zoom.ZoomAt(BIG, MapZoom::STEP, 30.f, 35.f);
	CHECK(zoom.Frame(BIG).cell == doctest::Approx(MapZoom::MAX_CELL));
}

TEST_CASE("map zoom: another fit keeps the zoom factor and clamps the kept pan") {
	MapFit wide = FIT;
	wide.cell = 1.f; // twice the cells across
	MapZoom zoom;
	zoom.ZoomAt(wide, 4.f, wide.area.cx(), wide.area.cy());
	zoom.Pan(wide, 1000.f, 1000.f); // far left in the wide view: past the left edge of FIT's
	const MapFrame f = zoom.Frame(FIT);
	CHECK(f.cell == doctest::Approx(8.f));
	CHECK(canvasX(f, 0.f) == doctest::Approx(FIT.area.x));
	CHECK(canvasY(f, 0.f) == doctest::Approx(FIT.area.y));
}
