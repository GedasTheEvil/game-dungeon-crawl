# Use string contains instead of find and npos

Status: draft 2026-11-13, size S, needs [cpp23-standard](solved/cpp23-standard.tooling.md) first.

Rank 3 of 5 in the C++23 set: small and fully mechanical, mostly test code.

## Problem

64 lines test for a substring or a character with `s.find(x) != std::string::npos` (or `== npos` for "not in"),
grep `\.find\([^)]*\) ?(!=|==) ?std::string::npos` in `src`, `tools`, `tests`. Most are in `tests/unit/`
(`wall_layer_test.cpp`, `rings_test.cpp`, `names_test.cpp`, ...), a few in `src` (`level/item_bag.cpp:535`,
`level/message_log.cpp:19`).

## Plan

* `s.find(x) != std::string::npos` → `s.contains(x)`; `== npos` → `!s.contains(x)`. Works for `std::string` and
  `std::string_view`, with a `char`, a literal or a string.
* Only where the position is not used. `auto pos = s.find(x); if (pos != npos) ... pos ...` stays.
* Same pass, already legal in C++20: prefix tests written as `s.rfind(x, 0) == 0` or `s.compare(0, n, x) == 0` →
  `s.starts_with(x)` (about 17 lines: `tests/unit/texts_test.cpp`, `wall_layer_test.cpp`, `level/leaderboard.cpp`,
  ...). Suffix tests → `ends_with`.
* Check the multi-line ones by hand: the grep misses a `find` whose argument has a `)` in it or that wraps.
* `make check`. No behaviour change, no play-test.
