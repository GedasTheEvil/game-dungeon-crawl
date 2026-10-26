# Smaller chest potions

Status: draft 2026-10-24, from the user ("The potions coming out of the chests are too large. I would scale them
down 2 times."). Needs nothing first. Implemented 2026-10-26, waits for the play-test:
`bin/game --play tests/play/smaller-chest-potions.txt`, open the containers.

## Done

* The table's `scale` column was not the drawn size: `DrawTreasureTile` (`src/world/dungeon_render.cpp`) raised every
  item to `CHEST_MIN_SCALE` (10), so the potions were drawn at 10, not 5. Halving the table would change nothing.
  Instead a potion skips that minimum and keeps its own scale (5, 5.2, 4.6, 5.5): about half the old 10 (all of
  them were 10; now the vessels' size ratios show). `POTION_MODELS` stays as it was.
* At half size, centred where it was, the potion sank into the chest (only the neck showed). It is lifted by
  `POTION_CHEST_LIFT` (2.5, half its height): it stands in the open chest.
* `detailScale` and `SLOT_SCALE` untouched. Nothing else reads a potion prototype's `scale` (the inventory overrides
  it); there are no dropped or thrown potions.
* `AMULET_SCALE` kept (4, raised to 10 on a chest, as before); its comment now says so.
* Scenario `tests/scenarios/solved/smaller_chest_potions.txt` (tests/levels/containers, every container holds a small health
  potion; `chest potion <id>` only adds a pickup, it draws nothing): an opened chest and coffer, compared with
  before-change screenshots.

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
