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
* Data in code vs data files: split off to [data-tables.draft.md](data-tables.draft.md).
* Per-kind `switch`es spread over many files: adding one monster or item kind touches how many places?
* Game state: one owner, or spread over globals/singletons?
* Tests: what is unit-testable without a window? What only runs as a scenario?

## Output

A short report: what holds up, what hurts, a ranked list of refactors (cost vs gain). Each worthwhile refactor gets
its own draft plan. No refactor without a clear gain for upcoming work.
