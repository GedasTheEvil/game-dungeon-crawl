#ifndef INVENTORY_LAYOUT_H
#define INVENTORY_LAYOUT_H

// Where the inventory screen's parts sit on the 160 x 100 canvas, and what a canvas point hits, without GL
// (docs/plan/solved/scenarios-to-unit-tests.md technique 5). Inventory draws them (ui/inventory.cpp).

#include "ui_layout.h"
#include "../world/item_bag.h"
#include "../world/items.h"

namespace InventoryLayout {
using ui::Rect;

constexpr Rect ITEMS_PANEL = {4, 13, 92, 72};
constexpr Rect DETAIL_PANEL = {100, 13, 56, 72};
constexpr Rect WIDE_BUTTON = {107, 16, 42, 7}; // potions: Drink
constexpr Rect EQUIP_BUTTON = {104, 16, 23, 7};
constexpr Rect UPGRADE_BUTTON = {129, 16, 23, 7};
// One grid for every tab: COLUMNS slots a row, as many rows as the group needs. VISIBLE_ROWS fit the panel; a group
// with more scrolls by whole rows (the wheel, the arrow keys past the last row shown), with a thin bar at the side.
constexpr int COLUMNS = 4;
constexpr int VISIBLE_ROWS = 2;
constexpr float SLOT_W = 19.f;
constexpr float SLOT_H = 22.f;
constexpr float SLOT_GAP = 3.f;
constexpr float TOP_ROW_Y = 52.f;
constexpr float GRID_W = COLUMNS * SLOT_W + (COLUMNS - 1) * SLOT_GAP;

// The group tabs across the top of the items panel, the same width as the grid.
constexpr float TAB_Y = 77.6f;
constexpr float TAB_H = 6.f;
constexpr float TAB_GAP = 2.f;

// The sort buttons right of the group tabs, one per SortOrder past Found (docs/plan/inventory-sort-orders.md).
constexpr float SORT_W[SORT_ORDER_COUNT] = {0.f, 6.f, 4.5f, 6.f};
constexpr float SORT_GAP = 1.f;
constexpr float SORTS_W = SORT_W[1] + SORT_W[2] + SORT_W[3] + 2 * SORT_GAP;

// In its tab's grid: the found items first, in `sort` order (tabOrder).
[[nodiscard]] int positionOf(const ItemBag& bag, SortOrder sort, int slot);
[[nodiscard]] int rowsOf(ItemGroup group);
[[nodiscard]] int rowOf(const ItemBag& bag, SortOrder sort, int slot);
// Its tab scrolled down by `scroll` rows.
[[nodiscard]] Rect slotRect(const ItemBag& bag, SortOrder sort, int slot, int scroll);
[[nodiscard]] bool slotVisible(const ItemBag& bag, SortOrder sort, int slot, int scroll);
// The tabs leave the right end of the row to the sort buttons.
[[nodiscard]] Rect tabRect(int group);
[[nodiscard]] Rect sortRect(int order);

// What a canvas point is over. twoButtons: the detail panel shows Equip and Upgrade (weapons, amulets), else one wide
// button. NO_HIT where it is over none of them.
constexpr int NO_HIT = -1;
enum class Button : signed char { None, Use, Upgrade };
struct Hit {
	int slot = NO_HIT; // a visible slot of the open tab
	int tab = NO_HIT;
	int sort = NO_HIT; // a SortOrder past Found
	Button button = Button::None;
};
[[nodiscard]] Hit hitAt(const ItemBag& bag, ItemGroup tab, SortOrder sort, int scroll, bool twoButtons, float x,
						float y);
} // namespace InventoryLayout

#endif
