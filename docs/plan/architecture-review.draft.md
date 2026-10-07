# Architecture review

Status: draft 2026-10-07.

## Idea

The game has grown a lot (bosses, weapons, attack timers, amulets, journal, riddles, many UI screens). Look over the
current architecture as a whole: does the split still make sense, or does it need a refactor?

## Snapshot (2026-10-07)

| Dir | Files | Lines |
|---|---|---|
| `src/world/` | 44 | ~6000 |
| `src/ui/` | 30 | ~5700 |
| `src/graphics/` | 25 | ~2400 |
| `src/entities/` | 14 | ~1900 |
| `src/state/` | 12 | ~1600 |
| `src/test/` | 2 | ~1200 |
| `src/input/` | 5 | ~960 |
| `src/core/` | 8 | ~700 |

Largest files: `test/scenario.cpp` (~1180), `ui/journal_view.cpp` (~1070), `ui/menu.cpp` (~950),
`ui/inventory.cpp` (~900), `state/assets.cpp` (~650).

## Questions to answer

* Module boundaries: is each dir one concern? `world/` is the biggest: dungeon, items, loot, monsters, level gen and
  the checker all live there. Split it?
* Dependencies: which way do they point? Does `world/` know about `ui/` or `graphics/`? Draw the include graph.
* Large files: split `journal_view`, `menu`, `inventory`, `scenario` by sub-screen or command?
* Data in code vs data files: see [Item and monster data](#item-and-monster-data).
* Per-kind `switch`es spread over many files: adding one monster or item kind touches how many places?
* Game state: one owner, or spread over globals/singletons?
* Tests: what is unit-testable without a window? What only runs as a scenario?

## Item and monster data

Question (2026-10-07): item labels, texts and stats live in the source. Move them to JSON or a similar file?

Today: already table-driven, as `constexpr std::array`s indexed by kind. One item's data is spread over several
tables in several files, though:

* `world/items.cpp`: `FILE_IDS` and `TEXTS`
* `world/loot.cpp`: `WEAPON_GRADES`
* `ui/inventory.cpp`: `POTION_COLORS`
* `world/monster_kinds.cpp`, `world/tile_defs.cpp`, `world/damage.h`, `world/poison.h`: the same pattern
* weapon stats and timers: somewhere else again

So the problem is mostly the **scatter**, not the format.

| | Tables in code (now) | JSON / data files |
|---|---|---|
| Checks | compiler: table size must match kind count | own validation at load: missing fields, typos, bad numbers |
| Deps | none | parser lib (or hand-written; there is an INI parser for settings already) |
| Edit loop | rebuild | edit and restart, no rebuild |
| Behaviour | data and code side by side | data only; new behaviour still needs code, so a new kind touches both |
| Translations | hard | easy (one file per language) |
| Modding | no | yes |

Leaning (to confirm in the review):

* First step, whatever the format: **one record per kind** (`ItemDef`: name, short name, texts, file id, grade,
  colour, stats) in one table instead of parallel arrays across files. Adding a kind then means one row.
* Data files only pay off with a concrete need: translations, modding, or tuning balance without rebuilds
  ([monster-balance](monster-balance.draft.md)). Without one, JSON brings a parser, load-time validation and
  error handling for little gain.
* If done: texts first (pure data, no behaviour). Keep enums for kinds in code, load by string id. The level checker
  (or a new check) validates the data files.

## Output

A short report: what holds up, what hurts, a ranked list of refactors (cost vs gain). Each worthwhile refactor gets
its own draft plan. No refactor without a clear gain for upcoming work.
