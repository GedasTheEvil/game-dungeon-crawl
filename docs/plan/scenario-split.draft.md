# Scenario runner: one place per command

Status: draft 2026-10-07. From the [architecture review](solved/architecture-review.md), refactor 4.

## Problem

`src/test/scenario.cpp` (1207 lines, 44 commits in 60 days) grows with almost every feature. A new command touches
three places in it: `parseLine` (482-732), `runInstant` (822-993) and often `fieldValue` (212-288, the `expect`
fields). The parser has no `Game()` but is not unit-tested; the docs table in [testing.md](../testing.md) is a fourth
place.

## Idea

* Split the file: parsing (no `Game()`, into the unit tests: every command parses, bad lines give a clear error),
  execution, and the `expect` fields.
* One table row per command (name, argument parser, run function) and per `expect` field (name, getter, help text), so
  a new command is one row. `docs_test` can check the testing.md tables against the rows.

## Open

* Whether the waiting commands (walk until, wait for) fit the same row shape.
