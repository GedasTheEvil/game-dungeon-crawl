# Inventory sorting: found first

Status: draft 2026-10-06, updated 2026-10-07. Implemented 2026-10-07 (see [Implementation](#implementation)), not
play-tested yet. Split off [inventory-overhaul.md](inventory-overhaul.md), which kept the `ItemKind` order.
Further orders: [inventory-sort-orders](../inventory-sort-orders.draft.md).

## New default: found first

Keep the `ItemKind` order, but move items not found yet to the end. Found items fill the first row and the first
lines of the grid without gaps, instead of being scattered between unfound slots. Within each group (found,
not found) the `ItemKind` order stays.

## Implementation

* `tabOrder(bag, group)` and `tabPosition(bag, kind)` (`src/world/item_bag.h`, unit tests in
  `tests/unit/items_test.cpp`): a tab's items, the found ones first. An item found and used up stays among the found.
* The inventory grid (`src/ui/inventory.cpp`) places, draws, hovers and scrolls its slots by `tabPosition`. A slot is
  still the `ItemKind` index, so the selection, the turntable angles and the scenario `select` do not change.
* The arrow keys and the number row (`1`-`=`) follow the grid's order, so a number key picks the item shown under it.
* Switching to a tab whose remembered slot was never found (a new game) selects its first found item.
* Applies to every tab, the Amulets tab too: it no longer has one row per amulet type. If that row layout is wanted
  back, `tabOrder` can keep the `ItemKind` order for the amulets.
