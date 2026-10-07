# Amulet upgrades

Status: draft 2026-10-07. Split off [amulets.draft.md](amulets.draft.md); for later, the way is not decided.

## Idea

Duplicates are allowed ([amulets](amulets.draft.md)), so spare amulets need a use. Two ways, decided later:

* **Combine:** X amulets of one type and tier make one of the next tier (e.g. 3 lesser -> 1 minor).
* **Boost:** like weapons (`weaponGrowthPercent` in `src/world/item_bag.cpp`), a duplicate adds to
  the amulet's bonus instead of a new slot.

## Open

* Which way; X per tier; can grand still grow; where it happens (inventory, a shrine).
