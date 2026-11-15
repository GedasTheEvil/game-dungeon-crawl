# Split the scenario runner's runInstant by command group

Status: solved 2026-11-16. Was: draft 2026-11-15, size M, needs nothing first.

## Problem

`runInstant` (`src/test/scenario.cpp:301`) has a cognitive complexity of 80 over 374 lines (`make complexity`,
plan complexity-hotspots): one `switch` over every instant command type, each case with its own nested checks.
`src/test/scenario.cpp` is the most changed file of the hard functions (43 commits since 2026-10-12): every new
scenario command adds a case here, and an agent reads the whole switch to add one.

## Plan

* Group the cases (setup: god, level, seed, resolution; items: give, equip, curse, ...; monsters: hurtboss,
  killboss, spawn, ...; player state: hp, stamina, ...) and move each group to a function
  `bool runItemCommand(const Command&)` etc., in the same file or a new `src/test/scenario_<group>.cpp`. `runInstant`
  keeps one `switch` that hands each type to its group, or a table.
* No behaviour change: `make check` and `make test` (every scenario) pass unchanged.
* Rerun `make complexity`: `runInstant` and each new function under 25; note the numbers here.
* docs/testing.md: where a new scenario command goes (its group function); Where is X row. 

## Result

`runInstant` (was 80, 374 lines) is now one `switch` that hands each command type to a group function in
`src/test/scenario.cpp`: `runSetupCommand`, `runInputCommand`, `runItemCommand`, `runProgressCommand`,
`runMonsterCommand`, `runPlayerCommand`, `runWorldCommand`, `runDataCommand`. `make complexity` (threshold 25): none
of them is listed any more, so each is under 25 (it lists only functions above 25; `runItemCommand` was 28 before
heka/page/learn moved to `runProgressCommand`). 106 functions above 25 before, 105 after. `make check`, `make test`
pass. docs/testing.md (where a new command goes) and the Where is X row updated.
