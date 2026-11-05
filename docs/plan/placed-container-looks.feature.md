# Placed container looks: a coffin (or any container) by hand

Status: implemented 2026-11-05, waiting for the user's play-test. Was: draft 2026-10-31, size L, needs nothing first. Left from [tomb-curse-containers](solved/tomb-curse-containers.md).

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

## Play-test

`bin/game --play tests/play/placed-container-looks.txt` (a shallow test level, where no coffin rolls):

* Walk east: a cursed coffin under the curse of Khentika (col 5, red text on its lid up close), a plain coffin, a
  coffer, an offering table, canopic jars, then a rolled container.
* Open the plain coffin: no curse. Open the cursed one: a curse.
* Campaign: `bin/game --level 47`, the room of "The last steward" (col 4, row 24): a cursed coffin by the inscription.
* `make run-editor LEVEL=content/levels/lvl47`: key `L` to the `Looks` layer; the coffins show on the map, a right
  click picks one, painting off a treasure is refused.

## Implementation notes (2026-11-05)

Point by point:

* Per-cell look: two fields of `Dressing` (`level.h`): `container` (`PLACED_UNSET` rolled, else a `Container`) and
  `lid` (`CoffinLid`: rolled, plain, inscribed; only a coffin's may be other than rolled). Level format v7: a
  `containers N` list at the end (`col row kind lid`); v6 and older load as rolled. Saves write the level block, so
  they keep it. `levelconvert` rewrote the 54 campaign levels and `tests/levels/subcells`; `ascii2level.py` writes an
  empty list.
* Stance: still rolled, through the new `containerStances` (`decor_scatter.cpp`), shared by the scatter and the
  checker: the coffin keeps the stances facing the camera or angled.
* A forced inscribed coffin is a cursed coffin as before (`TreasureLook::inscribed`); a coffin with a rolled lid keeps
  the depth's 40% roll; the other cells roll exactly as without the layer (unit test).
* Editor: a sixth layer, `Looks` (button after `Decals`, key `L` goes round six; the layer buttons got narrower to fit):
  `Rolled`, the seven containers, `Plain coffin`, `Cursed coffin`; icons `look_*.png` by `make_icons.py`; the map
  shows the looks in this layer, a right click picks one, the papyrus explains the brush or, in `Check` mode, the
  clicked cell's look. Painting a look off a treasure is refused (status line).
* `bin/levelcheck`: warns for a look on a cell without a treasure, a plain lid on a trapped treasure (a trapped coffin
  always shows the text; my addition) and a look with no stance that fits (today every cell has the stance facing the
  camera, so it cannot fire; kept for a future lengthwise kind or stance rule). `--map` prints a `containers`
  drawing, `--legend` the `container` lines.
* Docs: [levels.md](../levels.md), the editor readme (layers, the new `Looks layer` section, file format),
  glossary row **looks layer**, Where is X.
* Placed by hand (existing treasures, so no balance change): cursed coffins by The negative confession (lvl17 33,38),
  among the sleepers and canopic jars of the High Priest's tomb (lvl18 36,6) and by The last steward ("Let no one
  open it who has not read", lvl47 4,24); plain coffins in the burial chamber by Ptahhotep (lvl16 22,6), among the
  mummies of lvl47 (22,28) and by The heart scarab (lvl50 12,27). Every campaign level passes `bin/levelcheck` with no
  warnings.
* Tests: `tests/unit/placed_container_looks_test.cpp` (v7 round trip, v6 = rolled, bad lists, the forced look beats
  the roll and leaves the rest, the rolled lid, the stance filter by side walls, the checker's warnings, the `--map`
  drawing, the editor brushes, the campaign's placed coffins); scenario `tests/scenarios/placed_container_looks.txt`
  (screenshots; the plain coffin does not curse, the cursed one does).
