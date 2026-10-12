# Split Dungeon and Monster by feature

Status: draft 2026-10-12, ready (needs nothing first). From [dev-friction-review](solved/dev-friction-review.md),
part 3. Tooling and structure only, no gameplay change: once implemented and its checks pass, it goes straight to
`docs/plan/solved/` (AGENTS.md, Plans).

## Evidence (past sessions, 2026-10-12)

* Most-edited files (sessions that edited them, of 101): `dungeon.h` 46, `monster.h` 39, `monster.cpp` 34,
  `dungeon_base.cpp` 33, `dungeon_monsters.cpp` 32.
* Top co-edits: `dungeon.h` + `dungeon_monsters.cpp` 29, `monster.h` + `dungeon.h` 28, `monster.h` +
  `dungeon_monsters.cpp` 27. A monster ability touches the same 4-5 files every time (the bat dive:
  `gameplay_config.h`, `monster.h`, `monster_ai.cpp`, `dungeon.h`, `dungeon_monsters.cpp`).
* Median 11 src files edited per session.

## Idea

* **Monster:** each locomotion's state and its update in a file of its own: `Flight` (flyers), `Leap`, `Burrow`,
  `Charge`, `Climb`, beside `monster.h` (e.g. `entities/monster_flight.{h,cpp}`). `Monster` keeps one member per
  locomotion; the AI's decision for that locomotion moves with it from `monster_ai.cpp`. A new ability then adds a
  file instead of growing `monster.h` and `monster_ai.cpp`.
* **Dungeon:** `dungeon_monsters.cpp` holds flyer goals, walker paths, climbers and boss updates: split by the same
  locomotions where the code is separate. `dungeon.h` (~350 lines) declares every feature's state and helpers: move a
  feature's state into a struct of its own where it has one owner (dart traps, rock falls, travel), as `Darts` was.
* Not for its own sake: stop where a split needs a new interface between the parts. Measure: the files a monster
  ability touches (count with the next ability).
* Big UI files (`journal_view.cpp` ~1090, `inventory.cpp` ~970, `menu.cpp` ~950): left as they are. Each is one
  screen, edited in 18-27 sessions, not a search hot spot; split only a part that grows on its own.

## Checks

* `make check`, `make test`; no behaviour change (unit tests and scenarios unchanged).
* The map in [docs/development.md](../development.md#where-is-x) follows the moves, in the same commit.
* The layer lists in the makefile (`SIM_LIB_SOURCES`, `SIM_FILES`) take the new files.
