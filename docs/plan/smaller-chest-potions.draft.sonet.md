# Smaller chest potions

Status: draft 2026-10-24, from the user ("The potions coming out of the chests are too large. I would scale them
down 2 times."). Needs nothing first.

## Goal

A potion shown on an opened chest is drawn at half its current size.

## Where

* `src/world/items.cpp`, `POTION_MODELS`: the `scale` column (on a chest, `Item::scale`, about 5 now) halves:
  5 → 2.5, 5.2 → 2.6, 4.6 → 2.3, 5.5 → 2.75. Keep the ratios between the vessels.
* Leave `detailScale` (inventory detail view) alone. The inventory slots use their own `SLOT_SCALE`
  (`src/ui/inventory.cpp`), so they stay the same.
* Check whatever else reads a potion prototype's `scale`, e.g. a thrown or dropped potion, if any. Only the chest
  look was asked for.

## Open point

`AMULET_SCALE` (`src/state/assets.cpp`, 4) has the comment "like the potions (about 5)". The user asked only about
potions. Keep the amulet size, and fix the comment so it no longer says the potions are the same size.

## Tests

A scenario in `tests/scenarios/` that opens a potion chest (`chest potion <id>`) and takes a screenshot. Compare it
with a screenshot from before the change.
