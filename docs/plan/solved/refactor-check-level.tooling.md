# Split the level checker's checkLevel into its passes

Status: solved 2026-11-16. Was: draft 2026-11-15, size M, needs nothing first.

## Problem

`checkLevel` (`src/level/level_check.cpp:449`) has a cognitive complexity of 169 over 250 lines (`make
complexity`, plan complexity-hotspots): the entrance and exit checks, the start state, the Dijkstra over (cell,
opened colours), the reach of each cell, the goal, then the counts and warnings of what was reached and missed
(treasures, antidotes, heals, keys, pages, levers). Every checker plan reads it whole (7 commits since 2026-10-12;
checker plans are the riskiest: a wrong rule passes a level the game blocks).

## Plan

* Split it into its passes, each a function with a plain input and output: `checkEnds` (entrances, exits),
  `searchLevel` (the Dijkstra; returns dist, expanded, edges, the goal), `reachedCells`, `reportReached` (the counts
  and the warnings about reached content), `reportMissed` (unreached keys, pages, levers).
* No behaviour change: `bin/levelcheck content/levels/lvl*` prints the same report before and after (diff it), and
  the unit tests pass.
* Rerun `make complexity`: each pass under 40; note the numbers here. 

## Result

* `checkLevel` keeps the order of the checks and calls the passes: `checkEnds`, `searchLevel` (returns `Search`:
  dist, prev, prevMove, incoming, expanded, goal), `reachedCells`, `reportReached` (returns the antidote in reach),
  `reportMissed`, `checkSoftlocks`, `tracePath` (with `countPathMove`, `countPathCell`), `reportPath`.
* `bin/levelcheck` (plain and `--map`) over every campaign level and every test level (`tests/levels/`): the same
  output before and after (`cmp`).
* `make complexity`: `checkLevel` 169 → under 25, `tracePath` 34, every other pass under 25.
