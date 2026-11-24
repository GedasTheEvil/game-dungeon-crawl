#ifndef EDITOR_PALETTE_SCROLL_H
#define EDITOR_PALETTE_SCROLL_H

// The editor's palette scroll (plan editor-palette-scroll): the palette panel shows whole rows of slots; a layer with
// more rows scrolls them (the mouse wheel, or its scroll bar dragged, plan scrollbar-mouse-drag). The slot layout and
// the scroll math. Header only, no GL: the unit tests
// use it.

#include "../../src/level/ui/scroll_bar.h"
#include "../../src/level/ui/ui_layout.h"
#include <algorithm>

constexpr int PALETTE_COLUMNS = 9; // slots in a row
constexpr float SLOT_W = 7.2f;
constexpr float SLOT_H = 7.6f;
constexpr float SLOT_GAP = 0.6f;
constexpr float PALETTE_MARGIN = 2.f; // from the panel's bottom (and the same room above the top row)
constexpr float PALETTE_GRID_W = PALETTE_COLUMNS * SLOT_W + (PALETTE_COLUMNS - 1) * SLOT_GAP;
constexpr float PALETTE_BAR_ROOM = 0.6f; // the grid sits this much left of the panel's middle: the bar's room
constexpr float PALETTE_BAR_W = 0.6f;

// How many whole rows of slots fit in the panel.
[[nodiscard]] inline int paletteVisibleRows(const ui::Rect& panel) {
	int rows = 0;
	while (PALETTE_MARGIN + static_cast<float>(rows + 1) * SLOT_H + static_cast<float>(rows) * SLOT_GAP <= panel.h)
		rows++;
	return rows;
}

[[nodiscard]] inline int paletteRows(int slots) { return (slots + PALETTE_COLUMNS - 1) / PALETTE_COLUMNS; }

// The most rows the palette scrolls: 0 when all fit.
[[nodiscard]] inline int paletteMaxScroll(int slots, const ui::Rect& panel) {
	return std::max(0, paletteRows(slots) - paletteVisibleRows(panel));
}

// The bar right of the grid, over the rows shown (drawn only when the layer has more rows than fit).
[[nodiscard]] inline ui::ScrollBar paletteScrollBar(int slots, const ui::Rect& panel) {
	const int visible = paletteVisibleRows(panel);
	const float top = panel.y + panel.h - PALETTE_MARGIN;
	const float h = static_cast<float>(visible) * SLOT_H + static_cast<float>(visible - 1) * SLOT_GAP;
	const float gridRight = panel.cx() + PALETTE_GRID_W / 2 - PALETTE_BAR_ROOM;
	const float x = (gridRight + panel.x + panel.w) / 2 - PALETTE_BAR_W / 2;
	return {{x, top - h, PALETTE_BAR_W, h}, paletteRows(slots), visible};
}

// A slot's rect when the palette is scrolled `scroll` rows; rows grow downward from the panel's top.
[[nodiscard]] inline ui::Rect paletteSlotRect(int slot, int scroll, const ui::Rect& panel) {
	const float x0 = panel.cx() - PALETTE_GRID_W / 2 - PALETTE_BAR_ROOM;
	const int row = slot / PALETTE_COLUMNS - scroll;
	return {
		x0 + static_cast<float>(slot % PALETTE_COLUMNS) * (SLOT_W + SLOT_GAP),
		panel.y + panel.h - PALETTE_MARGIN - SLOT_H - static_cast<float>(row) * (SLOT_H + SLOT_GAP),
		SLOT_W,
		SLOT_H
	};
}

// The slot is on a visible row (so it is drawn and can be hit).
[[nodiscard]] inline bool paletteSlotShown(int slot, int scroll, const ui::Rect& panel) {
	return paletteSlotRect(slot, scroll, panel).within(panel);
}

// The slot under a point, or -1 (hidden rows never hit).
[[nodiscard]] inline int paletteSlotAt(float x, float y, int slots, int scroll, const ui::Rect& panel) {
	for (int slot = 0; slot < slots; slot++)
		if (paletteSlotShown(slot, scroll, panel) && paletteSlotRect(slot, scroll, panel).contains(x, y))
			return slot;
	return -1;
}

// The scroll that shows `slot`, from the current one (unchanged when it shows already), clamped.
[[nodiscard]] inline int paletteScrollTo(int slot, int scroll, int slots, const ui::Rect& panel) {
	const int row = slot / PALETTE_COLUMNS;
	const int visible = paletteVisibleRows(panel);
	if (slot >= 0 && row < scroll)
		scroll = row;
	else if (slot >= 0 && row >= scroll + visible)
		scroll = row - visible + 1;
	return std::clamp(scroll, 0, paletteMaxScroll(slots, panel));
}

#endif
