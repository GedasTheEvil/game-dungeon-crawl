# ItemBag::Load: drop the old-save branches, one reader per section

Status: solved 2026-11-16. Was: draft 2026-11-15, size S, needs nothing first.

## Problem

`ItemBag::Load` (`src/level/item_bag.cpp:481`) has a cognitive complexity of 101 over 127 lines (`make complexity`,
plan complexity-hotspots). Much of it reads old save layouts (a token that may be an old `mapX`, the `equipped`
fields added later, per-file and inline forms). The game has no real players yet: old saves can be ignored
(AGENTS.md).

## Plan

* Drop the branches for old save layouts; an old save that fails to load is fine (the loader reports it, the game
  does not crash). Bump the save version if there is one.
* Split what is left into one reader per section (counts, levels, found, stamps, equipped, rings).
* Unit test: a save written by `Save` loads back the same (round trip); a save from before is refused cleanly.
* Rerun `make complexity`: each part under 25; note the numbers here.

## Done

* Old layouts dropped: `Load` takes only `INV9`; any other tag (INV2 to INV8, the bare counts, a newer build's) sets
  failbit and the bag stays empty. The tag already is the save version, so no bump. The positional helpers
  (`legacyIndex`, `legacyItemAt`, `ORDERED_SAVE_SLOTS`, `OLD_SLOTS`, `SlotShift`, `parseInt`) are gone. The test
  fixtures `tests/saves/journal_all_creatures.sav` and `human_monster_notes.sav` were rewritten as INV9.
* Readers: `ReadRecords` (counts, levels, found, stamps are one record per item in INV9), `ReadTail` (equipped,
  worn, rings, older hand) and `Equip` (rings, amulet, weapon). One reader each for counts/levels/found/stamps does
  not apply: INV9 has them in one record.
* Tests: round trip (`the bag survives a save and a load`, rings, file-id suite) kept; old-layout tests replaced by
  one "refused cleanly" test (INV2, INV8, bare counts, INVX).
* Complexity (clang-tidy, threshold 0): Load 101 -> 7, ReadRecords 4, ReadTail 1, Equip 9 (all under 25).

