#include "inventory_layout.h"
#include <cmath>
#include <cstdio>
#include <utility>
#include <algorithm>

namespace InventoryLayout {

int positionOf(const GridOrder& order, int slot) {
	return static_cast<int>(std::find(order.begin(), order.end(), itemAt(slot)) - order.begin());
}

int rowsOf(ItemGroup group) { return (groupItems(group).count + COLUMNS - 1) / COLUMNS; }

int rowOf(const GridOrder& order, int slot) { return positionOf(order, slot) / COLUMNS; }

Rect slotRect(const GridOrder& order, int slot, int scroll) {
	const int position = positionOf(order, slot);
	const float x0 = ITEMS_PANEL.cx() - GRID_W / 2;
	const auto column = static_cast<float>(position % COLUMNS);
	const auto row = static_cast<float>(rowOf(order, slot) - scroll);
	return {x0 + column * (SLOT_W + SLOT_GAP), TOP_ROW_Y - row * (SLOT_H + SLOT_GAP), SLOT_W, SLOT_H};
}

bool slotVisible(const GridOrder& order, int slot, int scroll) {
	const int row = rowOf(order, slot);
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

namespace {
// "33.3", "20", "-50": one decimal unless whole.
std::string percent(double value) {
	const double tenths = std::round(value * 10.0);
	char buf[16];
	if (std::fmod(tenths, 10.0) == 0.0)
		snprintf(buf, sizeof(buf), "%d", static_cast<int>(tenths / 10.0));
	else
		snprintf(buf, sizeof(buf), "%.1f", tenths / 10.0);
	return buf;
}
} // namespace

std::string guardText(DamageType type, const Resistances& resist, const ArmorPoints& armor) {
	return percent(guardShare(type, resist, armor) * 100.0) + "%";
}

std::string guardTooltip(DamageType type, const Resistances& resist, const ArmorPoints& armor) {
	const auto t = std::to_underlying(type);
	const std::string name = DAMAGE_TYPE_NAMES[t];
	const std::string ward = percent(100.0 - static_cast<double>(resist[t])) + "%";
	if (type == DamageType::Fire)
		return name + ": " + ward + "\nthe fire guard's; armour does not stop fire";
	const std::string points = std::to_string(armor[t]) + "a";
	if (resist[t] == NORMAL)
		return name + ": " + points + "\n" + std::to_string(armor[t]) + " armour, no ward";
	return name + ": " + ward + " + " + points + "\nthe ward's " + ward + ", then " + std::to_string(armor[t]) +
		   " armour";
}

Hit hitAt(const GridOrder& order, int scroll, bool twoButtons, ShrineRow shrineRow, float x, float y) {
	Hit hit;
	for (ItemKind kind : order)
		if (slotVisible(order, itemIndex(kind), scroll) && slotRect(order, itemIndex(kind), scroll).contains(x, y))
			hit.slot = itemIndex(kind);
	for (int group = 0; group < ITEM_GROUP_COUNT; group++)
		if (tabRect(group).contains(x, y))
			hit.tab = group;
	for (int order = 1; order < SORT_ORDER_COUNT; order++)
		if (sortRect(order).contains(x, y))
			hit.sort = order;
	for (int i = 0; i < GUARD_COUNT; i++)
		if (guardRect(i).contains(x, y))
			hit.guard = i;
	if (!twoButtons) {
		if (WIDE_BUTTON.contains(x, y))
			hit.button = Button::Use;
	} else if (EQUIP_BUTTON.contains(x, y)) {
		hit.button = Button::Use;
	} else if (UPGRADE_BUTTON.contains(x, y)) {
		hit.button = Button::Upgrade;
	}
	const Rect offer = shrineRow == ShrineRow::OfferMerge ? OFFER_BUTTON : SHRINE_WIDE_BUTTON;
	if (shrineRow != ShrineRow::None && offer.contains(x, y))
		hit.button = Button::Offer;
	else if (shrineRow == ShrineRow::OfferMerge && MERGE_BUTTON.contains(x, y))
		hit.button = Button::Merge;
	return hit;
}

} // namespace InventoryLayout
