# Scenario command killmonster: the nearest monster dies as if killed by the player

Status: solved 2026-11-15. Was: draft 2026-11-15, size S, needs nothing first.

## Problem

`killboss` exists for bosses only. Testing what follows a kill of a common monster (XP, level-up, drops, the field
note, the death clip, a gate or a counter waiting for it) needs a scripted fight first.

## Plan

* `killmonster` (`src/level/scenario_script.cpp`, next to `killboss`): the living monster nearest the player dies as
  if the player killed it, through the same path as a killing hit (XP, field note, drop, death clip and sound, every
  kill hook). Fails if no monster lives. On the boss it does what `killboss` does.
* Optional argument `killmonster all`: every living monster on the level, nearest first (clearing a level for a
  scenario that tests something else).
* Docs: the command row in `docs/testing.md`.
* A scenario: `killmonster`, `expect xp > 0`, `expect journal_notes >= 1`.
