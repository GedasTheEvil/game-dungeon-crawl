# Inventory sort orders

Status: draft 2026-10-07, refined 2026-10-09 (decided, not implemented). Left open by
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
