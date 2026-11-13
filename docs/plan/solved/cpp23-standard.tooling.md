# Build with C++23

Status: solved 2026-11-13. Was: draft 2026-11-13, size S, needs nothing first.

Rank 1 of 5 in the C++23 set: the other four need it. Alone it changes no code; it lets new code use C++23.

## Problem

The build uses `-std=c++20` (`makefile:11` `CXXFLAGS`, `makefile:12` `TIDY_CPPFLAGS`). The tree uses no C++23
feature. The toolchain is ready: g++ 15.2, clang-tidy and clang-format 21.1.

## Plan

* `makefile`: `-std=c++23` in `CXXFLAGS` and `TIDY_CPPFLAGS`. Grep for another `-std=` (tools, scripts, docs,
  `LevelEditor/`, `ViewModel/`, `Play/`): on 2026-11-13 these two were the only ones.
* `make clean && make`, then `make tidy-all` and `make check`: fix every new warning (`-pedantic`, deprecations,
  clang-tidy parsing C++23 headers). Fix the code, don't silence it.
* `make test`: the scenarios still pass (same binary behaviour is expected; a difference is a bug to report).
* Compare the full build time before and after (`time make` after `make clean`, same `-j`): write both numbers in
  the commit message. More than 10% slower: report it, don't revert.
* `docs/development.md` (build and code style): name the standard (C++23) and add a short "C++23 in new code" list:
  `std::to_underlying` for an enum as an index, `string::contains`, `std::ranges::contains`, `std::unreachable()`
  after an exhaustive switch. The follow-up plans extend this list.
* `docs/glossary.md`: no new term expected.

## Not in this plan

No code rewrite. The follow-ups: [cpp23-enum-index](../cpp23-enum-index.tooling.draft.sonet.md),
[cpp23-string-contains](../cpp23-string-contains.tooling.draft.sonet.md),
[cpp23-idioms](../cpp23-idioms.tooling.draft.opus.md), [cpp23-print](../cpp23-print.tooling.draft.sonet.md).
