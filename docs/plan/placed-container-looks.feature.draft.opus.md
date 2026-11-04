# Placed container looks: a coffin (or any container) by hand

Status: draft 2026-10-31, size L, needs nothing first. Left from [tomb-curse-containers](solved/tomb-curse-containers.md).

Opus: level file format, editor, checker and hand placement in the campaign levels.

## Problem

A treasure's container is rolled per cell (`scatterTreasures`, `src/world/decor_scatter.cpp`: kind by
`CONTAINER_PERCENT[tier]`, stance, a coffin's curse text). The level format cannot force one: `Treasure` and
`TrappedTreasure` use both their attribute and value for the item ([editor readme](../../tools/editor/readme.md#tile-reference)).
So coffins and cursed coffins only appear where the depth roll puts them (level 10 on), never placed by hand: no
coffin in a burial chamber on purpose, no cursed coffin as a set piece next to its warning inscription.

## Plan

* A per-cell container look in the level file: unset (rolled as now), or a kind (`Container`, `src/world/containers.h`),
  for coffins also inscribed or plain. Where to keep it is the implementer's choice:
  * a new field of the Decor layer for treasure cells ([placed-decor](solved/placed-decor.md) already has unset /
    bare / picked), or
  * a new layer / field in the level file (bump the format version; old files load as unset).
* Stance stays rolled (it depends on the walls), only filtered by the forced kind (a coffin lies lengthwise).
* A forced inscribed coffin is a cursed coffin as now (`cursedCoffin`, curse on opening, one bonus item).
* Editor: pick the look on a treasure cell (palette like the decor one), show it in `Check` mode.
* `bin/levelcheck`: warn for a look on a cell without a treasure, and for a lengthwise kind (coffin) where no stance
  fits.
* Docs: [levels.md](../levels.md), the editor readme's tile reference.
* Place by hand: a few coffins (some inscribed) in the tomb levels (14-18, 46-50) and the Necropolis, next to an
  inscription or in a burial chamber; every campaign level still passes `bin/levelcheck` with no warnings.
* Unit tests: the format round trip (old file = unset), the forced look overrides the roll, the stance filter.
