# Scenario runner: one place per command

Status: draft 2026-10-07. Done 2026-10-07 (see [Implementation](#implementation)). From the
[architecture review](architecture-review.md), refactor 4.

## Problem

`src/test/scenario.cpp` (1207 lines, 44 commits in 60 days) grows with almost every feature. A new command touches
three places in it: `parseLine` (482-732), `runInstant` (822-993) and often `fieldValue` (212-288, the `expect`
fields). The parser has no `Game()` but is not unit-tested; the docs table in [testing.md](../../testing.md) is a fourth
place.

## Idea

* Split the file: parsing (no `Game()`, into the unit tests: every command parses, bad lines give a clear error),
  execution, and the `expect` fields.
* One table row per command (name, argument parser, run function) and per `expect` field (name, getter, help text), so
  a new command is one row. `docs_test` can check the testing.md tables against the rows.

## Implementation

Done 2026-10-07. Tooling only (no change to the game), so straight to solved.

* `src/test/scenario_script.{h,cpp}`: the commands, the `expect` fields, the operators and the parser, no `Game()`.
  It is in `liblevel.a` (the unit tests link it); `input/input.h` (key codes only) joined the level library's headers
  for it. The walk keys are a `Move` of the script; the runner maps them to `GameplayAction`.
* One row per command in `commandDefs()`: name, type, argument parser (shared by commands of one shape: `onOff`,
  `twoNumbers`, `numberAtLeast<N>`, ...), usage. One row per field in `fieldDefs()`. `parseScript` keeps the order
  rules (setup, then `level`).
* `src/test/scenario_fields.cpp`: the `expect` getters on the running game. `src/test/scenario.cpp` (1207 lines, now
  619): the runner. Running a command stays a switch on its type; `-Wswitch` flags a new type without a case. The
  waiting commands (`wait`, `walk`, `hold`) parse like the rest; only their run spans ticks, so the open question
  is moot.
* Tests: `tests/unit/scenario_script_test.cpp` (an example line per command, which must parse; bad arguments,
  unknown commands, the order rules, `compare`). `docs_test` fails when [testing.md](../../testing.md) misses a command
  or field, or lists one the code does not have (`click` got its own mention in the table for it).
* Error messages are now `usage: <name> <arguments>` for every command (a few said other things before).

Checks: `make` with no warnings, `make format-check`, `make tidy`, `make test` (unit tests and every scenario).
