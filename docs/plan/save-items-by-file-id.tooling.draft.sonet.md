# Save the bag and the potions drunk by stable item file id, not ItemKind position

Status: draft 2026-11-13, size M, needs nothing first.

## Problem

Items have stable ids for files (`ItemFileId`, `fileIdOf` / `itemFromFile`, `src/level/items.cpp:767-798`): the level
files and the equipped weapon, the worn amulet and the worn rings in a save use them. The bag's own arrays do not:

* `ItemBag::Save` (`src/level/item_bag.cpp:445`) writes `INV8 ITEM_KIND_COUNT`, then `counts`, `levels`, `found` and
  `stamps`, one value per `ItemKind` in enum order. `Load` maps slot `i` back to `itemAt(i)`.
* `RunStats` keeps `drunk` by `ItemKind` index (`src/level/run_stats.h:59`) and writes it as `index count` pairs
  (`dumpCounts`, `src/level/run_stats.cpp:41`).

So any `ItemKind` added anywhere but at the very end of the enum moves every later item in an existing save. The rule
in [development.md](../development.md#adding-a-tile-type-a-monster-or-an-item) ("a value in `ItemKind` at the end of its
group (save slots go by that order)") does not prevent it: the end of the weapons is before the potions, the amulets
and the rings.

It happened: plan sky-iron-rings (commit 8de811d9, 2026-11-09) put `GarnetSkyIron`, `LapisSkyIron`, ... inside the
ring group, while the bag's tag stayed `INV8` (since 709c70f4, 2026-10-30). Every `INV8` save from those ten days
reads each ring count after the first gem into the wrong ring. The same change gave the worn rings' file ids an
explicit escape (`SKY_IRON_RING_FILE_ID`, `items.h:189`) so they kept reading; the counts beside them were missed.
Before that, the tags `INV5` and `INV7` each needed a hand-written shift table (`SHIFT_BEFORE_INV5`,
`SHIFT_BEFORE_INV7`, `OLD_SLOTS`, `savedSlot`: `item_bag.cpp:16-58`).

The game has no players yet (AGENTS.md, old saves), but the user's and the scenarios' saves outlive builds, and the next
weapon, potion or amulet type does it again, silently: nothing fails.

The leaderboard already does it right: `potion_<name>=count` lines (`src/level/leaderboard.cpp:275`).

## Plan

1. `ItemBag::Save`: tag `INV9`, then the number of records and one record per item kind that has anything to keep
   (count > 0, found, level > 1 or a stamp): `type id count level found stamp` with `fileIdOf(kind)`. Then the
   equipped, worn and ring ids and the older hand as today.
2. `ItemBag::Load`: read `INV9` records through `itemFromFile`; skip an unknown pair (an item from a newer build).
   Keep the `INV2` ... `INV8` paths untouched for older saves.
3. `RunStats`: write `drunk` as `type id count` (a new tag after the existing ones, or a version step in the `RUN`
   line, implementer's choice); read the old index form for old saves.
4. A fixed save in `tests/saves/` written by the new code (a few weapons with levels, potions, an amulet, rings on both
   hands, a drunk count) and a unit test that loads it and checks each item. A later `ItemKind` insertion keeps this
   test green; with positional slots it would fail.
5. development.md, "Adding ... an item": drop "(save slots go by that order)"; a new kind only needs a free file id.

## Risk

Low: one format, read and written in one file each, with the old paths kept. `tests/unit/save_load_safety_test.cpp`
and `ring_accessories_test.cpp:183` (save and load of worn rings) cover the round trip.

## Verify

* `make check`.
* The new test; then, as a check by hand, move one `ItemKind` value in a scratch build: the fixed save still loads
  the same items (revert after).
