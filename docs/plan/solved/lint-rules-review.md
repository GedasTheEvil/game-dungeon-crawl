# Lint rules that fit the code

Status: done 2026-10-07 (no game change). From the user: the linter must not get in the way of good code.

## Idea

Look through the last week of work (git log, commits, NOLINT lines, code bent to satisfy a check) and find where the
agents wanted to write something sensible that `make tidy`, the compiler warnings or `make format` did not allow.
Change the rules (`.clang-tidy`, `.clang-format`, `CXXFLAGS`, `tools/check_*.sh`) so that code passes as written.

## Seen so far

* `-Wmissing-field-initializers` (GCC) against `readability-redundant-member-init` (clang-tidy): a designated
  initializer row that skips a `std::optional` member warns in GCC unless the member has an initializer, and tidy
  calls `{}` redundant. Worked round with `= std::nullopt` (`MonsterKind`, `src/world/monster_kinds.h`,
  [data-tables](data-tables.md)). One of the two should go.
* `NOLINT(clang-analyzer-optin.performance.Padding)` on table structs ordered to read (`MonsterKind`, `WeaponDef`):
  padding of a static table does not matter. Consider turning the check off.

## To do

* Grep the last week's diffs for `NOLINT`, workarounds, reordered fields, casts or helpers added only for a check.
* For each: drop or tune the check, or keep it with a reason.
* Check that `make tidy` and `make format-check` stay clean after the change.

## Done

Looked back to the tidy setup (2026-09-28): the NOLINTs, the warnings the agents' sessions hit and how they were
fixed. `make`, `make unit`, `make tidy`, `make format-check` clean after.

* `-Wno-missing-field-initializers` in `CXXFLAGS`: a designated-initializer row leaves out what it does not need, and
  the rest is value-initialized anyway. `MonsterKind::poison` and `spit` are plain `std::optional` members again.
* `clang-analyzer-optin.performance.Padding` off; the two NOLINTs (`MonsterKind`, `WeaponDef`) gone.
* `performance-inefficient-string-concatenation` off: settings_ini.cpp had a `join({...})` helper only for it, now
  `a + b + c`.
* `bugprone-narrowing-conversions`: int to float no longer checked (`WarnOnIntegerToFloatingPointNarrowingConversion`).
  It was the bulk of the warnings and the reason `static_cast<float>` went from 336 to 730 uses. Float to int,
  double to float and signed/unsigned stay. The existing casts stay: removing them by hand is churn, and
  `static_cast<float>(a) / b` is not the same as `a / b`.

Kept, the fixes made the code better or caught real slips: integer-division, unchecked-optional-access,
assignment-in-if-condition, no-automatic-move, implicit-bool-conversion, use-bool-literals, `-Wformat-security`.

Left as is: the `(void)x;` lines for unused GLUT callback parameters (`-Wunused-parameter`); a nameless parameter
does the same, either is fine.
