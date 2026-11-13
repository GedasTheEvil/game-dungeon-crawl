# Index arrays by enum with std::to_underlying

Status: draft 2026-11-13, size M, needs [cpp23-standard](cpp23-standard.tooling.draft.sonet.md) first.

Rank 2 of 5 in the C++23 set: the most call sites and the most noise removed.

## Problem

About 300 array subscripts are `[static_cast<size_t>(...)]` (grep `\[static_cast<size_t>\(` in `src`, `tools`,
`tests`; 382 `static_cast<size_t>` in all). Roughly half of them cast an enum to index a table:
`SKILLS[static_cast<size_t>(id)]`, `type.resist[static_cast<size_t>(DamageType::Pierce)]`,
`blessing_left_ms[static_cast<size_t>(God::Isis)]`. The cast names a type the reader does not care about and hides
which subscripts are enums and which are plain `int`.

All the `enum class` types in `src` have an unsigned underlying type (`std::uint8_t`, `unsigned`) except one
`int8_t`, one `signed` and one `bool`, so `std::to_underlying` gives an index that needs no further cast.

## Plan

* Replace `static_cast<size_t>(e)` with `std::to_underlying(e)` (`<utility>`) where `e` is an enum and the result
  is an index or a size. Subscripts first; also loop bounds and `array<T, static_cast<size_t>(E::Count)>` sizes.
* Leave alone: casts of `int`, `float` or another non-enum (this plan does not touch them); enums with a signed
  underlying type if the result then needs a second cast (keep the old cast there).
* Find the enum sites by type, not by guess: build after each batch; `std::to_underlying` on a non-enum does not
  compile, so a wrong replacement fails loudly. Work per directory (`src/level`, `src/sim`, `src/ui`, ...), one
  commit each or one in all.
* Open point (implementer's choice): a table indexed by one enum at 10+ sites may read better with a tiny accessor
  that exists already (`skillDef(id)`, `weaponDef(kind)`); prefer the accessor over a new cast at those sites. No
  new wrapper type (`EnumArray`) in this plan.
* `make tidy` may flag `bugprone-narrowing-conversions` or an implicit widening at a few sites: fix the site (keep
  the old cast there), don't change `.clang-tidy`.
* `docs/development.md` code style: "an enum as an index: `std::to_underlying(e)`, never `static_cast<size_t>`".
* `make check`. No behaviour change: the unit tests are the proof; no play-test.

## Done when

`grep -rE 'static_cast<size_t>\(' src tools tests` shows no enum operand (sample 30 of the rest by hand), and the
count is in the commit message (before, after).
