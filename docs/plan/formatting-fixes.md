# Formatting fixes

Status: implemented 2026-10-01. `Courage` comments moved above the enumerators; `make format-check`
(`tools/check_format.sh`) added, documented in AGENTS.md and docs/development.md. No other unstable spot found.

## Problem

`make format` (clang-format 18.1.3, `.clang-format`: LLVM base, tabs, 120 columns) is not idempotent on some code:
each run flips it between two layouts, so a file shows as changed after every format run, with no edit.

Found so far (2026-10-01, every file `make format` touches, formatted twice): one spot, the `Courage` enum in
`src/entities/monster.h`. Its trailing comment runs over two lines:

```cpp
	Coward,	  // afraid of traps (spikes, death traps, a rock fall not yet fallen): a walker stops at their edge, a
			  // walk-jumper leaps over them
	Reckless, // walks straight through them and takes their damage, cut by MonsterType::trapDamagePct
```

Aligned with the `Reckless` comment, the first line is close to the 120-column limit (with tabs at width 4). One pass
aligns it, the next drops the alignment because the line would be too long, the pass after that aligns it again.

## Fix

* Rewrite the spots that flip: a comment above the enumerator instead of a trailing one, or a shorter comment that
  fits on one line.
* Add a `make format-check` target: format every file into a temp copy twice (`clang-format --assume-filename=<file>`
  so it reads `.clang-format`) and fail if the two passes differ, or if the first pass differs from the file on disk.
  Then a flip-flop shows up when it is written, not a few commits later. Run it in `make test` or document it in
  AGENTS.md next to `make format` / `make tidy`.
* Check other small format issues found along the way, for example multi-line trailing comments on struct
  members and enumerators elsewhere (now stable, but one longer word can tip them over).

## Not

* No clang-format version bump or style change only for this: fix the code, keep the style.
