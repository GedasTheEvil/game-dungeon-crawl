# Cognitive complexity report: the hardest functions as refactor candidates

Status: solved 2026-11-15. Was: draft 2026-11-15, size M, needs nothing first.

## Problem

A controlled study of Claude Code on cleaner and messier copies of the same repositories (fewer static-analysis
violations, lower cognitive complexity; arXiv 2605.20049, 2026) found the same pass rate, but 7-8% fewer tokens and
34% fewer file revisits on the cleaner code. Formatting and lint style barely matter; hard-to-follow functions do. We
don't know which of our functions are the hard ones.

## Plan

* A report, not a gate: `make complexity` (or `tools/code_map.py complexity`) runs clang-tidy with only
  `readability-function-cognitive-complexity` (threshold about 25; pick it from the spread) over every source
  through the load gate, and prints the functions above it, most complex first: score, file:line, name, line count.
  Not in `.clang-tidy`, not in `make check`.
* Run it once, then cross the list with where agents spend effort: files read and edited most
  (`tools/friction_report.py`, the transcripts) and tidy's fires by file. A complex function nobody touches is not
  a candidate.
* Write the top 5-10 candidates into this plan (score, why it is hard: long switch, nested state, flags), each with
  the split proposed. One `tools/plan.py new` draft per refactor worth doing (kind tooling); behaviour must not change,
  the unit tests stay green.
* Docs: the command in docs/development.md (Code quality tools), one line.

## Result (2026-11-15)

`make complexity [THRESHOLD=N]` (`tools/code_map.py complexity`): clang-tidy with only
`readability-function-cognitive-complexity` over the tidy sources, through the load gate, about 9 minutes with other
builds running. Threshold 25 (clang-tidy's; a lower one finds nothing more): 106 functions over it in 170 sources. The
spread: 2 over 150, 10 over 70, 40 over 44. A report, not a check.

Crossed with the effort: commits per file since 2026-10-12 (`git log --name-only`; the transcripts' edit counts were
too sparse and use the paths from before the src/ split) and tidy's fires by file. Score × commits, the top:

| Function | Score | Lines | Commits | Why it is hard | Draft |
|---|---|---|---|---|---|
| `runInstant` (src/test/scenario.cpp) | 80 | 374 | 43 | one switch over every scenario command, nested checks per case | [refactor-scenario-run-instant](../refactor-scenario-run-instant.tooling.draft.sonet.md) |
| `main` (tools/level/levelcheck.cpp) | 75 | 152 | 26 | options, load, report, scripts and ranking in one | [refactor-levelcheck-main](../refactor-levelcheck-main.tooling.draft.sonet.md) |
| `DrawDetails` (src/ui/inventory.cpp) | 80 | 162 | 22 | every item class's panel in one, nested per class | [refactor-inventory-details](../refactor-inventory-details.tooling.draft.sonet.md) |
| `UpdateMonsters` (src/sim/dungeon_monsters.cpp) | 103 | 145 | 12 | one loop: death, motion, attack, each a nested `if` with `continue` | [refactor-update-monsters](../refactor-update-monsters.tooling.draft.opus.md) |
| `checkLevel` (src/level/level_check.cpp) | 169 | 250 | 7 | ends, search, reach, warnings in one; checker plans are the riskiest | [refactor-check-level](../refactor-check-level.tooling.draft.opus.md) |
| `Load` (src/level/item_bag.cpp) | 101 | 127 | 11 | old save layouts (no real players: can go) | [refactor-item-bag-load](../refactor-item-bag-load.tooling.draft.sonet.md) |
| `runCommands` (src/test/scenario.cpp) | 37 | 67 | 43 | the wait states; small, left with runInstant's split | none |
| `Draw` (src/ui/journal_view.cpp) | 39 | 121 | 33 | long but flat drawing | none |
| `drawGameplay` (src/graphics/draw.cpp) | 35 | 206 | 34 | long, flat: a run of effects | none |
| `scatterSurfaces` (src/level/decor_scatter.cpp) | 181 | 119 | 4 | the most complex, but hardly touched | none |

`fieldValue` (scenario_fields.cpp, 53) is a flat switch over fields: easy to read, its score is the count of cases.

