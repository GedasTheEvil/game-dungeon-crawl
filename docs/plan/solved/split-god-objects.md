# Split Dungeon and Monster by feature

Status: implemented and solved 2026-10-16 (no gameplay change, see [Implemented](#implemented-2026-10-16)). Drafted 2026-10-12. From [dev-friction-review](dev-friction-review.md),
part 3. Tooling and structure only, no gameplay change: once implemented and its checks pass, it goes straight to
`docs/plan/solved/` ([plans](../../plans.md#solved)).

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
  feature's state into a struct of its own where it has one owner (dart traps, rock falls, travel), as `Darts` was (there was no `Darts` struct yet: this plan made it).
* Not for its own sake: stop where a split needs a new interface between the parts. Measure: the files a monster
  ability touches (count with the next ability).
* Big UI files (`journal_view.cpp` ~1090, `inventory.cpp` ~970, `menu.cpp` ~950): left as they are. Each is one
  screen, edited in 18-27 sessions, not a search hot spot; split only a part that grows on its own.

## Checks

* `make check`, `make test`; no behaviour change (unit tests and scenarios unchanged).
* The map in [docs/development.md](../../development.md#where-is-x) follows the moves, in the same commit.
* The layer lists in the makefile (`SIM_LIB_SOURCES`, `SIM_FILES`) take the new files.

## Implemented (2026-10-16)

* **Monster:** `entities/monster_{flight,leap,burrow,charge,climb}.{h,cpp}`: the state struct in the header (included
  by `monster.h`), its `Monster` members in the source (moved from `monster_ai.cpp` and `monster.cpp`: the lifts,
  the climb clip). The declarations stay in `Monster`: moving them out would need a new interface (the members use
  `x`, `enter`, `bite`, the links). `monster_ai.cpp` 500 -> 178 lines, `monster.h` 369 -> 316.
* **Dungeon:** `UpdateMonsters` (`dungeon_monsters.cpp`, 497 -> 270 lines) dispatches to `updateWalker`
  (`dungeon_walkers.cpp`: what blocks a walker, leaps, flight of cowards, climbers' paths), `updateFlyer`
  (`dungeon_flight.cpp`, with `flightGoal`, `flightShaft`), `updateBurrower` (`dungeon_burrow.cpp`, with
  `burrowTarget`) and `updateCharger` (`dungeon_charge.cpp`).
* **Dungeon state:** `DartTraps` (`dungeon_darts.h`: plates, volleys, darts), `GateTravel` (nested in `Dungeon`).
  Rock falls: left as they are, one `std::vector<Motion>` shared in shape with the opening gates, nothing to group.
* A new locomotion now: a `monster_<name>.{h,cpp}` pair, a `dungeon_<name>.cpp` with its `update<Name>`, one line
  in `UpdateMonsters`; still a member and declarations in `monster.h` and `dungeon.h`. Measure with the next ability
  ([mummy-boss](mummy-boss.md)).
* Checks: `make check` (tidy clean), 335 unit tests and the 3 scenarios unchanged and passing.
