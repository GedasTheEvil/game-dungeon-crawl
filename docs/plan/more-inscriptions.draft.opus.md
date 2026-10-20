# More inscriptions, from level 1 on

Status: draft 2026-10-12. Needs [skill-hieroglyphs](solved/skill-hieroglyphs.md) first (the copy without the
skill). From the user.

## Idea

Once an inscription the player cannot read is copied into the journal ("No idea what it says"), nothing stops
inscriptions in the early levels: they are a promise of the hieroglyph skill. So place them from level 1 on, and
more than one per level, also in the lower levels. Reading them later (learning the skill fills the copies in) pays
off the early ones.

## What changes

* Campaign rule: drop `INSCRIPTIONS_FROM_LEVEL` (`src/world/inscriptions.h`), its `levelcheck` warning
  (`checkInscriptions`, `tools/level/levelcheck.cpp`), the unit test's "before level 6" check
  (`tests/unit/inscriptions_test.cpp`) and the sentence in `docs/inscriptions.md` and `docs/levels.md`.
* Placement: levels 1-5 get inscriptions (suggested 1-2 each), the rest one or two more (suggested 2-3 per level).
  Same rules as in [hieroglyph-cartouches](solved/hieroglyph-cartouches.md): a reachable floor cell, mostly on or
  near the path; hints only, never needed. Edit the level files in `content/levels/` (an object `15 ID 0` on the cell, by
  hand or in the editor), never the ASCII drawings: the level files are the source of truth.
* Texts: new entries in `content/inscriptions/*.txt` with `G:` glyphs, then rebuild the atlas
  (`tools/textures/inscriptions.py`). Real texts (Pyramid Texts, Coffin Texts, the Shipwrecked Sailor, Sinuhe,
  Wenamun, workmen's ostraca from Deir el-Medina, the Amarna hymn to the Aten) and hints. Early-level hints suit
  what the player meets there (the first key and gate, the spike pit, the first mimic, the scarab king of level 5).
* Atlas capacity: ids stop at 64 (`INSCRIPTION_ATLAS_GRID` 8). Around 70-90 inscriptions need more: a larger grid
  (12 x 12 at 256 px: 3072 px square) or 128 px cells in the same 2048 px; implementer's choice, keep the id = cell
  rule.

## Tests

* The campaign unit test: every level 1-30 has at least one inscription, each id has a text and is used once.
* `bin/levelcheck content/levels/lvl*` with no warnings.
