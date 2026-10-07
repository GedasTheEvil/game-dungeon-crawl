# Inventory sorting options

Status: draft 2026-10-06, updated 2026-10-07. Split off [inventory-overhaul.md](solved/inventory-overhaul.md),
which keeps the `ItemKind` order.

## Idea

Sorting options for the items in an inventory tab.

## New default: found first

Keep the `ItemKind` order, but move items not found yet to the end. Found items fill the first row and the first
lines of the grid without gaps, instead of being scattered between unfound slots. Within each group (found,
not found) the `ItemKind` order stays.

## Open

* Further orders: by name, by level, by count, recently found.
* Where the choice sits (a small control on the items panel) and whether it is saved per tab.
