# Split levelcheck's main: options, per-level report, ranking

Status: draft 2026-11-15, size S, needs nothing first.

## Problem

`main` of `tools/level/levelcheck.cpp:178` has a cognitive complexity of 75 over 152 lines (`make complexity`,
plan complexity-hotspots): option parsing (`--map`, `--quiet`, `--legend`, `--script`), loading each level, the
per-level report, the path scripts and the ranking, all in one. The file changed in 26 commits since 2026-10-12.

## Plan

* `parseOptions(argc, argv)` returns an options struct (or an error); `loadEntries`, `printReport(entry)`,
  `writeScript(entry)` and `printRanking(entries)` take the parts; `main` calls them in order.
* No behaviour change: `bin/levelcheck content/levels/lvl*`, `--quiet`, `--map`, `--legend` and `--script DIR` print
  the same before and after (diff them).
* Rerun `make complexity`: `main` and each part under 25; note the numbers here. 
