#ifndef EDITOR_MAP_VIEW_H
#define EDITOR_MAP_VIEW_H

// The editor's map view (plan editor-zoom): the zoom (1x to 3x) and the pan offset, and the mapping between cells and
// canvas coordinates. View state only, not saved in the level. Header only, no GL: the unit tests use it.

#include "../../src/ui/ui_layout.h"
#include "../../src/world/level.h"
#include "../../src/world/level_check.h"
#include <algorithm>
#include <cmath>

constexpr float EDITOR_CELL = 1.8f; // canvas units per cell at 1x
constexpr float EDITOR_MAP_X = 6.f;
constexpr float EDITOR_MAP_Y = 8.5f;
constexpr ui::Rect EDITOR_MAP_AREA = {EDITOR_MAP_X, EDITOR_MAP_Y, LEVEL_WIDTH* EDITOR_CELL, LEVEL_HEIGHT* EDITOR_CELL};
constexpr int MIN_ZOOM = 1;
constexpr int MAX_ZOOM = 3;

class MapView {
  public:
	[[nodiscard]] int zoom() const { return zoomLevel; }
	// The bottom left corner of the shown window, in cells (fractions allowed).
	[[nodiscard]] float originCol() const { return originX; }
	[[nodiscard]] float originRow() const { return originY; }
	[[nodiscard]] float cellSize() const { return EDITOR_CELL * static_cast<float>(zoomLevel); }
	[[nodiscard]] float visibleCols() const { return static_cast<float>(LEVEL_WIDTH) / static_cast<float>(zoomLevel); }
	[[nodiscard]] float visibleRows() const { return static_cast<float>(LEVEL_HEIGHT) / static_cast<float>(zoomLevel); }

	[[nodiscard]] ui::Rect cellRect(int col, int row) const {
		return {EDITOR_MAP_X + (static_cast<float>(col) - originX) * cellSize(),
				EDITOR_MAP_Y + (static_cast<float>(row) - originY) * cellSize(), cellSize(), cellSize()};
	}

	// The cell under a canvas point, {} (col -1) outside the map area.
	[[nodiscard]] CellPos cellAt(float x, float y) const {
		if (!EDITOR_MAP_AREA.contains(x, y))
			return {};
		CellPos cell = {static_cast<int>(std::floor(originX + (x - EDITOR_MAP_X) / cellSize())),
						static_cast<int>(std::floor(originY + (y - EDITOR_MAP_Y) / cellSize()))};
		return LevelGrid::inBounds(cell.col, cell.row) ? cell : CellPos{};
	}

	// Sets the zoom (clamped) so that the map point (col, row), in cells, stays where it is on the screen: at
	// (fracX, fracY) of the map area, 0..1.
	void setZoom(int z, float col, float row, float fracX, float fracY) {
		zoomLevel = std::clamp(z, MIN_ZOOM, MAX_ZOOM);
		originX = col - fracX * visibleCols();
		originY = row - fracY * visibleRows();
		clamp();
	}

	// Moves the window by cells.
	void pan(float cols, float rows) {
		originX += cols;
		originY += rows;
		clamp();
	}

	// Moves the window so that the map point (col, row) is at its centre.
	void centerOn(float col, float row) {
		originX = col - visibleCols() / 2;
		originY = row - visibleRows() / 2;
		clamp();
	}

  private:
	int zoomLevel = MIN_ZOOM;
	float originX = 0.f;
	float originY = 0.f;

	void clamp() {
		originX = std::clamp(originX, 0.f, static_cast<float>(LEVEL_WIDTH) - visibleCols());
		originY = std::clamp(originY, 0.f, static_cast<float>(LEVEL_HEIGHT) - visibleRows());
	}
};

#endif
