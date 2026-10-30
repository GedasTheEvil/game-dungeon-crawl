# Level legend: glyph fixes

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Tooling and docs only: once checks pass, solved in the same commit (docs/plans.md, Solved, exception).

* An unknown key colour is drawn as `q` (Rerek's glyph) and an unknown gate colour as `Q` (the giant cobra's):
  `src/world/tile_defs.cpp` vs `src/world/monster_kinds.cpp`; documented in `docs/levels.md`. Pick free glyphs
  (AGENTS.md, Glyphs) and update the doc.
* `docs/levels.md` lists the ascii2level structure glyphs without sand `,` and quicksand `%`, while
  `levelcheck --legend` prints `structure #.~=,%`. Fix the doc.
* A unit test that no two legend entries share a glyph.
