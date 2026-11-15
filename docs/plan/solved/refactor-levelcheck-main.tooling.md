# Split levelcheck's main: options, per-level report, ranking

Status: solved 2026-11-16. Was: draft 2026-11-15, size S, needs nothing first.

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

## Result

Done 2026-11-16. Complexity after (clang-tidy, threshold 1): `main` 13 (was 75), `parseOptions` 9, `loadEntries` 8,
`printLegend` 12, `printRanking` 12, `printReport` 14 (unchanged), `writeScript` under 2. All under 25.
Output of `lvl*` plain, `--quiet`, `--map`, `--legend`, `--script DIR` (files too), usage errors and exit codes is
identical before and after. One edge differs: files are now loaded after all options are parsed, so with an unreadable
file before a bad option the usage error no longer comes after that file's error line (stderr order only).
