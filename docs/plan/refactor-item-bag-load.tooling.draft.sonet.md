# ItemBag::Load: drop the old-save branches, one reader per section

Status: draft 2026-11-15, size S, needs nothing first.

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
