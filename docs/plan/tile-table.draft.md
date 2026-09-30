# Stage 3: tile and monster definition tables

Status: planned 2026-09-30, not started. Stage 3 of the [code structure review](code-structure-review.draft.md).
Evidence: [the audit](code-structure-review-audit.draft.md) (Per-type if-chains, Tile descriptions, Duplication).
Comes after [stage 2](solved/rules-out-of-ui.md) (unit tests, item ids GL-free).

## Why

* A new tile kind or monster touches 12-20 places (teleporter, boss gate and boss monster in the audit), over C++,
  Python and docs. Several places were missed and nobody noticed: no map symbol for the teleporter and the boss gate,
  no gem name for the boss lock, no level_gen support.
* The same per-type facts are written down several times and already disagree: `TILE_COUNT = 14` in the editor, the
  lock colours twice in `tile_info.cpp`, the checker's glyph table copied by hand into `ascii2level.py` (the round
  trip is lossy), two sources of "is a boss" (`isBossMonster` hard-codes one id, `MonsterType::isBoss()` reads
  `BOSS_DEFS`), `MONSTER_CHARS` that must follow the `MonsterTypeId` order.
* `attr` / `value` mean something else per type; gate and rock state are bare 0 / 1 / 2.

## Design

Data in `liblevel` (GL-free, shared by the game and every tool); behaviour stays where it runs, dispatched by type
with a compile-time check that every type is handled.

* `src/world/tile_defs.{h,cpp}`: `enum DungeonTileType` gets `TileTypeCount`; `TILE_DEFS[TileTypeCount]`, one row per
  type: name, editor label and description, glyph(s), solid rule, standable, what `attr` / `value` mean (none, lock
  colour, gate type, monster type, pair id, state), checker hazard cost. `static_assert` on the row count.
* Typed accessors instead of raw `attr` / `value`: `gateState(t)` / `setGateState` with `enum class GateState
  { Closed, Open, Opening }` (the same 0 / 1 / 2 on disk), `rockState`, `lockColour(t)`, `teleportPair(t)`,
  `monsterType(t)`.
* `src/world/monster_kinds.{h,cpp}`: one row per `MonsterTypeId` with the GL-free facts: name, glyph, checker threat,
  `boss` flag, editor text. `isBossMonster` and `MonsterType::isBoss()` both read it; `MONSTER_DEFS` (stats, models)
  and `BOSS_DEFS` (minion rules) stay in `assets.cpp`, keyed by the same id.
* Lock colours: one table (name, gem name, colour for the map / HUD / editor) with the boss lock as its last row.
* Glyphs: `renderLevel` and a new `levelcheck --legend` print from the tables; `ascii2level.py` reads that legend
  instead of its own copy, so the glyph round trip is exact (add the missing glyphs, fix the `k` clash).
* Teleporter pairs: an index built on level load (`pair id -> two cells`) instead of the full scan per call.

## Steps

1. **Unit tests first:** level round trip, `isSolidTile`, `renderLevel` → `ascii2level` round trip for every test
   level and `levels/lvl*`, teleporter pairing. They pin today's behaviour.
2. `TileTypeCount`, `TILE_DEFS` and `monster_kinds`; the editor (`tile_info`, `TILE_COUNT`) and the checker
   (`MONSTER_CHARS`, `monsterThreat`, glyphs) read them.
3. Typed accessors; replace the raw 0 / 1 / 2 in `dungeon_mechanisms.cpp`, `level_check.cpp` and the magic
   `attr` / `value` in `DrawTreasureTile`.
4. Lock colour table; the map, HUD, level gem and editor palettes read it.
5. `levelcheck --legend`; `ascii2level.py` uses it. Rebuild the campaign from the ASCII sources and check the files
   are byte-identical.
6. Dispatch: `Dungeon::Draw` and `Interact` become a `switch` over `DungeonTileType` (one function per type, the
   54-line Door branch split), with `-Wswitch` catching a missing type. The decor skip lists read a `TILE_DEFS` flag.
7. Teleporter index.
8. Docs: `docs/levels.md` and the editor readme tables generated from, or checked against, the tables.

## Checks

`./levelcheck levels/lvl*` output byte-identical before and after each step; the campaign levels unchanged on disk;
all scenarios; the new unit tests.

## Open questions

* Store the draw / interact functions in the table (a render-side table indexed by type) or keep a `switch`? The
  `switch` keeps GL out of `liblevel` and the compiler checks coverage; the table is one place to look.
* Generate `docs/levels.md` tables from the code, or only check them in a test?
