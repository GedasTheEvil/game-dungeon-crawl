# Split the inventory's DrawDetails per item class

Status: solved 2026-11-16. Was: draft 2026-11-15, size S, needs nothing first.

## Problem

`Inventory::DrawDetails` (`src/ui/inventory.cpp:951`) has a cognitive complexity of 80 over 162 lines (`make
complexity`, plan complexity-hotspots): one function draws the detail panel of every item class (ring, amulet,
weapon, potion, the unknown item, the shrine buttons) with nested checks per class. `src/ui/inventory.cpp` changed
in 22 commits since 2026-10-12.

## Plan

* One function per item class: `drawRingDetails`, `drawAmuletDetails`, `drawWeaponDetails`, `drawPotionDetails`;
  `DrawDetails` keeps the shared parts (the model, the shadow, the name, the lore or the shrine buttons) and calls
  the one for the selected slot.
* No visual change: a scenario screenshot of each class before and after (inventory open on a ring, an amulet, a
  weapon, a potion), the same pixels.
* Rerun `make complexity`: each function under 25; note the numbers here. 

## Result

Done. `DrawDetails` 80 -> 6. Per class (members of `Inventory`, PascalCase like the rest of the class): `DrawRingDetails`
8, `DrawAmuletDetails` 10, `DrawWeaponDetails` 21, `DrawPotionDetails` 0. The kind line (red, under the name) moved
to `DetailKind` 15. Nothing in `src/ui/inventory.cpp` from this plan is above 25 (`make complexity`).
Pixels: `tests/scenarios/solved/inventory_details.txt` (ring, amulet, weapon, potion, unknown) screenshots before and after are
byte-identical.
