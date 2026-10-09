# Inventory sort orders

Status: implemented 2026-10-09, not play-tested yet. Draft 2026-10-07, refined 2026-10-09. Left open by
[inventory-sorting](solved/inventory-sorting.md), which made "found first" the default.

## Decided (2026-10-09)

Three orders on top of the default. Not found items stay at the end in all of them.

| Button | Order |
|---|---|
| `[a-z]` | by name, A to Z (`itemText(kind).name`) |
| `[*]` | by strength, strongest first: weapons by damage now (`weaponDamage` at its level), amulets by tier (grand first, then the `ItemKind` order), potions by grade (large / greater before small / lesser, then the `ItemKind` order) |
| `[new]` | recently found first |

* **Control:** small buttons on the items panel, `[a-z] [*] [new]`, in the [UI](../ui.md) style. A click picks that
  order, the active one is lit; a click on the active one goes back to found first.
* **Not saved:** every tab starts in found first when the game starts.
* **Recently found** needs a stamp per item: a counter set by `ItemBag::Find` / `Add` on every pickup, so a fresh
  potion comes up. The stamps go in the bag's save (a new `INV` version, older saves
  load with no stamps: found first among them).
* `tabOrder` (`src/world/item_bag.h`) takes the order; the grid, the arrow keys and the scenario `select` follow it as
  today. A scenario command to pick an order, and unit tests per order.

## Implemented (2026-10-09)

* `SortOrder` and `tabOrder(bag, group, sort)` in `src/world/item_bag.h`; the inventory keeps one order per tab.
* The buttons sit right of the group tabs, which got narrower (14.6 instead of 19.75). The font's `*` is a dot, so the
  strength button shows a gold diamond. Hover names the order ("Sort by name", "Strongest first", "Last found first",
  on the lit one "Back to found first").
* Strength for potions: large health, large stamina and greater resistance first, all the others after, each part in
  `ItemKind` order.
* Stamps: `ItemBag::Add` counts up (`Stamp`), an amulet upgrade stamps the new one. Saves are `INV6` now (the stamps
  after the found flags); `INV5` loads with no stamps.
* Scenario command `sort found|name|strength|recent`, unit tests per order, `tests/scenarios/inventory_sort.txt`.
