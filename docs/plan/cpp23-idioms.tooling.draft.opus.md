# C++23 ranges, optional and unreachable where they read better

Status: draft 2026-11-13, size M, needs [cpp23-standard](solved/cpp23-standard.tooling.md) first.

Rank 4 of 5 in the C++23 set: fewer sites and each one is a judgement call, so the gain per hour is lower than
[cpp23-enum-index](cpp23-enum-index.tooling.draft.sonet.md).

## Problem

Some C++20-style code has a shorter C++23 form. None of it is wrong; change a site only when the new form is
clearly easier to read. Counts are from 2026-11-13 and are upper bounds.

| Today | C++23 | Where |
|---|---|---|
| `for (size_t i = 0; i < v.size(); i++)` that uses both `i` and `v[i]` | `for (auto [i, x] : std::views::enumerate(v))` | 108 `size_t` index loops, 144 `int i` loops in `src`; only a part fits |
| a loop over two arrays by one index | `std::views::zip(a, b)` | among the same loops |
| `std::find(...) != v.end()`, `std::ranges::find(...) != v.end()` | `std::ranges::contains(v, x)` | about 13 |
| `std::accumulate(begin, end, init, op)` | `std::ranges::fold_left(v, init, op)` | 4 |
| `if (opt) return f(*opt); return {};` | `opt.transform(f)`, `.and_then(f)`, `.or_else(f)` | 230 `std::optional` uses; few fit |
| `default:` or a trailing `return {};` after a switch that covers every enumerator | `std::unreachable()` | grep switches over enums |
| `bool load(..., std::string& error)` | `std::expected<T, std::string>` | about 3 |

## Plan

* Order: `ranges::contains` and `fold_left` (mechanical), then `std::unreachable`, then loops, then `optional`,
  then `expected`.
* Loops: `enumerate` yields a signed index (`difference_type`). Skip a loop when the body then needs a cast it did
  not need before, when it writes `i` or steps by more than 1, or when `i` indexes more than what `zip` can pair.
  Skip the hot paths unless the generated code is the same (rendering: `src/graphics/`, `src/render/`; the per-frame
  simulation loops): `-O3` usually removes the view, but check one with `perf` or the frame-time scenario before
  doing the rest.
* `std::unreachable()` is undefined behaviour when reached: use it only after a switch over an enum whose value
  cannot come from a file or a save unchecked. Parsed values (levels, saves, config) keep a real error path.
* `optional` chains: only where it removes an `if` and the lambda is one line. No nested chains.
* `expected`: only if all callers get simpler; otherwise leave the 3 sites.
* Each kind is its own commit, so one can be reverted alone.
* `docs/development.md`: extend the "C++23 in new code" list with the forms kept, and one line on when not to use
  `enumerate` (the rules above).
* `make check`, `make test`. No behaviour change, no play-test.

## Open points

* Compile time: `<ranges>` views in headers cost build time. Use them in `.cpp` files; measure `time make` after
  `make clean` before and after, and say so in the commit.
* How many loops fit is unknown until read; if fewer than about 20 do, drop the loop part and say so.
