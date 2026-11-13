# Docs checks: heading anchors, backticked identifiers, zone table

Status: draft 2026-11-13, size M, needs nothing first.

From a docs cross-check on 2026-11-13 ([docs-stale-refs](docs-stale-refs.tooling.draft.sonet.md) lists what it found
by hand). The project prefers a check to a one-off fix; three classes of doc drift have no guard today:

* `tools/plan.py links` fixes and reports broken file targets, not `#anchor`s: `docs/level-format.md:105` links
  `#zones`, a heading that file does not have.
* `tests/unit/docs_test.cpp` checks only the glossary's Code column (names exist in `src/`, paths exist). Backticked
  identifiers elsewhere in `docs/*.md`, `readme.md`, `tools/editor/readme.md` and `AGENTS.md` are unchecked:
  `waterSpeed` in `docs/level-format.md:67` is gone from `src/`.
* The `docs/levels.md` zone table (Levels, Monsters, Boss columns) is checked against nothing. `docs_test.cpp` checks
  only monster threats; the per-level table is currently right, but the zone table already differs from `ZONE_FAUNA`
  in `src/level/campaign.h`.

## Plan

* Anchors: extend `plan.py links` (or a small `tools/docs_check.py` called from `make check`) to check
  `(file.md#anchor)` links in `docs/*.md`, `readme.md`, `AGENTS.md`, `tools/editor/readme.md` against GitHub-style
  heading slugs (lowercase, drop `` ` * `` and punctuation, spaces to `-`). Solved and won't-do plans stay unchecked.
* Identifiers: a docs_test case (or the same script) that takes every backticked CamelCase / camelCase / UPPER_CASE
  token in those files (not `glossary.md`, `remodeling.md`, plans) and requires it in `src/`, `tools/`, `tests/unit/`
  or the makefile. Needs a short allow list for words that are examples (`CamelCase`, `camelBack`, `UPPER_CASE`,
  `FORCE`, `LevelEditor`, `ViewModel`: root launcher scripts). A sweep on 2026-11-13 with that rule found only
  `waterSpeed` as real.
* Zone table: a docs_test case that reads the `| N | Zone | a-b | ... | Boss |` rows of `docs/levels.md` and checks
  the level range against `ZONES` (first, last) and the boss against the zone's boss kind label; optionally that each
  `ZONE_FAUNA` monster label (or its plural) appears in the row's Monsters cell. The level files already match
  `ZONES` ranges and `zone` slugs (checked by hand 2026-11-13).

## Verification

* Each new check fails on the current tree for `#zones` and `waterSpeed` before [docs-stale-refs](docs-stale-refs.tooling.draft.sonet.md)
  is done, and passes after.
* `make check`.
