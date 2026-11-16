# Remove the bare section divider comments

Status: draft 2026-11-16, size S, prio low, needs nothing first.

## Problem

`src/` and `tests/` have about 393 bare divider lines (`//=====...`) in 40 files, in five lengths (60 to 128 `=`),
plus one doubled `////=====`. None has a title. They sit between most functions (e.g.
`src/graphics/dungeon_render_sun.cpp`), so they mark every function boundary and say nothing about how the file is
organised. Braces and clang-format already show where a function ends.

## Plan

* Do it when the working tree is clean (no other work in progress on `src/`): it touches many files and would
  collide with parallel edits.
* Delete every line that is only a divider, whatever its length:
  `grep -rlE '^\s*/{2,}={20,}\s*$' src tests | xargs sed -i -E '/^\s*\/{2,}={20,}\s*$/d'`.
  Check nothing else matched first (`grep -rnE '/{2,}={5,}' src tests`: a line with text plus `=====` is not a
  divider; leave it and report it).
* A comment directly under a divider that described the function stays as it is.
* `make format` collapses any double blank lines left behind; then `make check` (no tidy warnings, unit tests pass).
* Look for dividers outside `src/` and `tests/` (`tools/`, shader files, `docs/development.md` code style) and
  remove them the same way; if `docs/development.md` mentions the dividers as style, drop that line.
* One commit with only this change, no version bump (tooling).
