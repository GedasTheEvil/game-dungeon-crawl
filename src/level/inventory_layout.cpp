#include "inventory_layout.h"
#include <algorithm>

namespace InventoryLayout {

int positionOf(const ItemBag& bag, SortOrder sort, int slot) { return tabPosition(bag, itemAt(slot), sort); }

int rowsOf(ItemGroup group) { return (groupItems(group).count + COLUMNS - 1) / COLUMNS; }

int rowOf(const ItemBag& bag, SortOrder sort, int slot) { return positionOf(bag, sort, slot) / COLUMNS; }

Rect slotRect(const ItemBag& bag, SortOrder sort, int slot, int scroll) {
	const int position = positionOf(bag, sort, slot);
	const float x0 = ITEMS_PANEL.cx() - GRID_W / 2;
	const auto column = static_cast<float>(position % COLUMNS);
	const auto row = static_cast<float>(rowOf(bag, sort, slot) - scroll);
	return {x0 + column * (SLOT_W + SLOT_GAP), TOP_ROW_Y - row * (SLOT_H + SLOT_GAP), SLOT_W, SLOT_H};
}

bool slotVisible(const ItemBag& bag, SortOrder sort, int slot, int scroll) {
	const int row = rowOf(bag, sort, slot);
	return row >= scroll && row < scroll + VISIBLE_ROWS;
}

Rect tabRect(int group) {
	const float w = (GRID_W - SORTS_W - ITEM_GROUP_COUNT * TAB_GAP) / ITEM_GROUP_COUNT;
	return {ITEMS_PANEL.cx() - GRID_W / 2 + static_cast<float>(group) * (w + TAB_GAP), TAB_Y, w, TAB_H};
}

Rect sortRect(int order) {
	float x = ITEMS_PANEL.cx() + GRID_W / 2 - SORTS_W;
	for (int o = 1; o < order && o < SORT_ORDER_COUNT; o++)
		x += SORT_W[o] + SORT_GAP;
	return {x, TAB_Y, SORT_W[std::clamp(order, 0, SORT_ORDER_COUNT - 1)], TAB_H};
}

Hit hitAt(const ItemBag& bag, ItemGroup tab, SortOrder sort, int scroll, bool twoButtons, float x, float y) {
	Hit hit;
	for (int slot = 0; slot < ITEM_KIND_COUNT; slot++)
		if (itemGroup(itemAt(slot)) == tab && slotVisible(bag, sort, slot, scroll) &&
			slotRect(bag, sort, slot, scroll).contains(x, y))
			hit.slot = slot;
	for (int group = 0; group < ITEM_GROUP_COUNT; group++)
		if (tabRect(group).contains(x, y))
			hit.tab = group;
	for (int order = 1; order < SORT_ORDER_COUNT; order++)
		if (sortRect(order).contains(x, y))
			hit.sort = order;
	if (!twoButtons) {
		if (WIDE_BUTTON.contains(x, y))
			hit.button = Button::Use;
	} else if (EQUIP_BUTTON.contains(x, y)) {
		hit.button = Button::Use;
	} else if (UPGRADE_BUTTON.contains(x, y)) {
		hit.button = Button::Upgrade;
	}
	return hit;
}

} // namespace InventoryLayout
