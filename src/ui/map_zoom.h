#ifndef MAP_ZOOM_H
#define MAP_ZOOM_H

// The draft map's zoom and pan (plan map-symbols-and-zoom). At 1x the sketch fits the explored part on the sheet (a
// MapFit); zoomed in, the view pans within that 1x view, never past it. The fit grows as the player explores, so the
// zoom and pan are kept as asked and clamped to the fit of the frame. View state only, not saved. Header only, no GL:
// the unit tests use it.

#include "../level/ui_layout.h"
#include <algorithm>

// The 1x view: the drawing area on the sheet, the canvas units per cell, the map point (in cells) at its centre.
struct MapFit {
	ui::Rect area;
	float cell;
	float centreI, centreJ;
};

// Where cell (0, 0)'s corner lands on the canvas, and the canvas units per cell.
struct MapFrame {
	float originX, originY, cell;
};

class MapZoom {
  public:
	static constexpr float STEP = 1.25f; // a wheel notch or a +/- press
	static constexpr float MAX_CELL = 9.f; // canvas units per cell at the most; a fit bigger than that does not zoom

	[[nodiscard]] float Zoom(const MapFit& fit) const { return std::clamp(zoom, 1.f, maxZoom(fit)); }

	[[nodiscard]] MapFrame Frame(const MapFit& fit) const {
		const float cell = fit.cell * Zoom(fit);
		const float ci = fit.centreI + std::clamp(panI, -reachI(fit, cell), reachI(fit, cell));
		const float cj = fit.centreJ + std::clamp(panJ, -reachJ(fit, cell), reachJ(fit, cell));
		return {fit.area.cx() - ci * cell, fit.area.cy() - cj * cell, cell};
	}

	// Zooms by `factor`, keeping the map point under the canvas point (x, y) where it is.
	void ZoomAt(const MapFit& fit, float factor, float x, float y) {
		const MapFrame before = Frame(fit);
		const float u = (x - before.originX) / before.cell;
		const float v = (y - before.originY) / before.cell;
		zoom = std::clamp(Zoom(fit) * factor, 1.f, maxZoom(fit));
		const float cell = fit.cell * zoom;
		panI = (fit.area.cx() - x) / cell + u - fit.centreI;
		panJ = (fit.area.cy() - y) / cell + v - fit.centreJ;
		clamp(fit);
	}

	// Moves the sketch by (dx, dy) canvas units, as a drag does.
	void Pan(const MapFit& fit, float dx, float dy) {
		const float cell = Frame(fit).cell;
		clamp(fit);
		panI -= dx / cell;
		panJ -= dy / cell;
		clamp(fit);
	}

	void Reset() { *this = MapZoom{}; }

  private:
	float zoom = 1.f;
	float panI = 0.f, panJ = 0.f; // the view's centre from the fit's, in cells

	[[nodiscard]] static float maxZoom(const MapFit& fit) { return std::max(1.f, MAX_CELL / fit.cell); }
	// How far the centre may move from the fit's, in cells, for the view to stay within the 1x view.
	[[nodiscard]] static float reachI(const MapFit& fit, float cell) {
		return std::max(0.f, fit.area.w / 2 * (1.f / fit.cell - 1.f / cell));
	}
	[[nodiscard]] static float reachJ(const MapFit& fit, float cell) {
		return std::max(0.f, fit.area.h / 2 * (1.f / fit.cell - 1.f / cell));
	}
	void clamp(const MapFit& fit) {
		zoom = Zoom(fit);
		const float cell = fit.cell * zoom;
		panI = std::clamp(panI, -reachI(fit, cell), reachI(fit, cell));
		panJ = std::clamp(panJ, -reachJ(fit, cell), reachJ(fit, cell));
	}
};

#endif
