#ifndef VIEW_WINDOW_H
#define VIEW_WINDOW_H

// A block of cells: cols x rows from (col0, row0).
struct CellRect {
	int col0 = 0;
	int row0 = 0;
	int cols = 0;
	int rows = 0;
	[[nodiscard]] bool contains(float x, int row) const {
		return row >= row0 && row < row0 + rows && x >= static_cast<float>(col0) && x < static_cast<float>(col0 + cols);
	}
	[[nodiscard]] CellRect grown(int cells) const {
		return {col0 - cells, row0 - cells, cols + 2 * cells, rows + 2 * cells};
	}
};

// The gameplay window round the player: WIDTH x HEIGHT cells from (firstCol(), originRow). Monsters spawn in it and
// are seen in it (the journal), whatever the window size. Dungeon::Draw's frame puts cell (originCol, originRow) at
// the origin; the column left of it belongs to the window too.
struct ViewWindow {
	static constexpr int WIDTH = 10;
	static constexpr int HEIGHT = 6;
	int originCol = 0;
	int originRow = 0;
	[[nodiscard]] int firstCol() const { return originCol - 1; }
	[[nodiscard]] CellRect cells() const { return {firstCol(), originRow, WIDTH, HEIGHT}; }
	[[nodiscard]] bool contains(float x, int row) const { return cells().contains(x, row); }
};

// How far the drawn window may reach past the gameplay window on each side: a very wide window does not draw half the
// level.
constexpr int DRAWN_EXTRA_COLS = 12;
constexpr int DRAWN_EXTRA_ROWS = 4;

// The cells to draw: the gameplay window, widened to cover what the camera sees of the level (plus a margin cell for
// models that reach past their cell), at most DRAWN_EXTRA_* past it. clip: projection x modelview (column-major, as
// glGet gives it) in Dungeon::Draw's frame, where a cell is tileSize wide and cell (originCol, originRow) sits at the
// origin. The level lies between z 0 (the rock face) and z -tileSize (the back wall).
[[nodiscard]] CellRect drawnWindow(const ViewWindow& view, const float clip[16], float tileSize);

#endif
