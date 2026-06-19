# Tidy Plan — Overview

`make tidy` runs `clang-tidy` with the checks in `.clang-tidy`. Total warnings: **151** across **18 source files**.

## Warning summary

| Count | Check | Part |
|------:|-------|------|
| 60 | `bugprone-narrowing-conversions` | [Part 3](part-3-narrowing-conversions.md) |
| 36 | `readability-implicit-bool-conversion` | [Part 2](part-2-bool-literals.md) |
| 36 | `modernize-use-bool-literals` | [Part 2](part-2-bool-literals.md) |
| 9 | `readability-identifier-naming` | [Part 4](part-4-identifier-naming.md) |
| 4 | `bugprone-implicit-widening-of-multiplication-result` | [Part 5](part-5-performance-and-widening.md) |
| 2 | `performance-type-promotion-in-math-fn` | [Part 5](part-5-performance-and-widening.md) |
| 2 | `clang-analyzer-deadcode.DeadStores` | [Part 1](part-1-critical-analyzer-issues.md) |
| 1 | `clang-analyzer-valist.Uninitialized` | [Part 1](part-1-critical-analyzer-issues.md) |
| 1 | `bugprone-integer-division` | [Part 1](part-1-critical-analyzer-issues.md) |

> Note: `readability-implicit-bool-conversion` and `modernize-use-bool-literals` fire on the **same 36 lines** — fixing one fixes both.

## Parts

| File | Scope | Warnings |
|------|-------|---------|
| [Part 1 — Critical analyzer issues](part-1-critical-analyzer-issues.md) | va_list, dead stores, integer division | 4 |
| [Part 2 — Bool literals](part-2-bool-literals.md) | Replace `0`/`1` with `false`/`true` in bool contexts | 36 locations |
| [Part 3 — Narrowing conversions](part-3-narrowing-conversions.md) | Explicit casts for int/double→float narrowing | 60 |
| [Part 4 — Identifier naming](part-4-identifier-naming.md) | Local `const` variables must use `UPPER_CASE` | 9 |
| [Part 5 — Performance and widening](part-5-performance-and-widening.md) | `std::cosf`/`std::sinf`, explicit `static_cast` widening | 6 |

## Suggested order

Fix in order: Part 1 → Part 5 → Part 4 → Part 2 → Part 3.
Parts 1, 4, and 5 are small and mechanical. Parts 2 and 3 are high-volume but also mechanical.
`make tidy-fix` can auto-apply many of these; run it after each part to verify.
