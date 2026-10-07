# Lint rules that fit the code

Status: draft 2026-10-07, from the user: the linter must not get in the way of good code.

## Idea

Look through the last week of work (git log, commits, NOLINT lines, code bent to satisfy a check) and find where the
agents wanted to write something sensible that `make tidy`, the compiler warnings or `make format` did not allow.
Change the rules (`.clang-tidy`, `.clang-format`, `CXXFLAGS`, `tools/check_*.sh`) so that code passes as written.

## Seen so far

* `-Wmissing-field-initializers` (GCC) against `readability-redundant-member-init` (clang-tidy): a designated
  initializer row that skips a `std::optional` member warns in GCC unless the member has an initializer, and tidy
  calls `{}` redundant. Worked round with `= std::nullopt` (`MonsterKind`, `src/world/monster_kinds.h`,
  [data-tables](solved/data-tables.md)). One of the two should go.
* `NOLINT(clang-analyzer-optin.performance.Padding)` on table structs ordered to read (`MonsterKind`, `WeaponDef`):
  padding of a static table does not matter. Consider turning the check off.

## To do

* Grep the last week's diffs for `NOLINT`, workarounds, reordered fields, casts or helpers added only for a check.
* For each: drop or tune the check, or keep it with a reason.
* Check that `make tidy` and `make format-check` stay clean after the change.

## Open

* How far back: one week, or since the tidy rules were set up.
