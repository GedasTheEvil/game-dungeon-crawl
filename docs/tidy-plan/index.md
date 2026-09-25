# Tidy Plan — Overview

`make tidy` runs `clang-tidy` with the checks in `.clang-tidy`. Total warnings: **149** across **17 source files** (re-counted 2026-09-25, after the MD3 model loader rewrite and the PNG texture loader).

## Warning summary

| Count | Check | Part |
|------:|-------|------|
| 60 | `bugprone-narrowing-conversions` | [Part 3](part-3-narrowing-conversions.md) |
| 37 | `readability-implicit-bool-conversion` | [Part 2](part-2-bool-literals.md) |
| 37 | `modernize-use-bool-literals` | [Part 2](part-2-bool-literals.md) |
| 12 | `readability-identifier-naming` | [Part 4](part-4-identifier-naming.md) |
| 2 | `performance-type-promotion-in-math-fn` | [Part 5](part-5-performance-and-widening.md) |
| 1 | `clang-analyzer-valist.Uninitialized` | [Part 1](part-1-critical-analyzer-issues.md) |

Gone since the first count: `clang-analyzer-deadcode.DeadStores` (2) and `bugprone-integer-division` (1), fixed in Part 1;
`bugprone-implicit-widening-of-multiplication-result` (4), removed with the old `.mdl` loader in `ani.cpp`.

> Note: `readability-implicit-bool-conversion` and `modernize-use-bool-literals` fire on the **same 37 lines** — fixing one fixes both.

## Parts

| File | Scope | Warnings |
|------|-------|---------|
| [Part 1 — Critical analyzer issues](part-1-critical-analyzer-issues.md) | va_list, dead stores, integer division | 1 left (of 4) |
| [Part 2 — Bool literals](part-2-bool-literals.md) | Replace `0`/`1` with `false`/`true` in bool contexts | 37 locations |
| [Part 3 — Narrowing conversions](part-3-narrowing-conversions.md) | Explicit casts for int/double→float narrowing | 60 |
| [Part 4 — Identifier naming](part-4-identifier-naming.md) | Local `const` variables must use `UPPER_CASE` | 12 |
| [Part 5 — Performance and widening](part-5-performance-and-widening.md) | `std::cosf`/`std::sinf` (widening items gone) | 2 left (of 6) |

## Suggested order

Fix in order: Part 1 → Part 5 → Part 4 → Part 2 → Part 3.
Parts 1, 4, and 5 are small and mechanical. Parts 2 and 3 are high-volume but also mechanical.
`make tidy-fix` can auto-apply many of these; run it after each part to verify.
