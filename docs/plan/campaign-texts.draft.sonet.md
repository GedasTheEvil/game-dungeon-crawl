# Campaign texts: enough texts for 40-60 levels

Status: draft 2026-10-21, from the user. Needs first: [campaign-zones](campaign-zones.draft.opus.md) (the level count
and the zone themes the texts follow).

## Idea

The texts were written for 35 levels. The rebuilt campaign ([denser-levels](denser-levels.draft.opus.md)) has 40-60
levels, inscriptions from level 1 on, and several forks. Write the texts first, so the generator
([campaign-levelgen](campaign-levelgen.draft.opus.md)) and the rebuild only place them.

## What changes

* **Inscriptions** (merged in from the more-inscriptions draft): drop `INSCRIPTIONS_FROM_LEVEL`
  (`src/world/inscriptions.h`), its `levelcheck` warning (`checkInscriptions`, `tools/level/levelcheck.cpp`), the unit
  test's "before level 6" check (`tests/unit/inscriptions_test.cpp`) and the sentence in `docs/inscriptions.md` and
  `docs/levels.md`. New entries in `content/inscriptions/*.txt` with `G:` glyphs, 2-3 per level (1-2 on the first
  zone's levels), then rebuild the atlas (`tools/textures/inscriptions.py`). Real texts (Pyramid Texts, Coffin Texts,
  the Shipwrecked Sailor, Sinuhe, Wenamun, workmen's ostraca from Deir el-Medina, the Amarna hymn to the Aten) and
  hints that suit the zone. Keep the boon share of [inscription-boons](solved/inscription-boons.md).
* **Atlas capacity:** ids stop at 64 (`INSCRIPTION_ATLAS_GRID` 8). 100-180 inscriptions need a larger grid (for
  example 16 x 16 at 128 px in 2048 px); keep the id = cell rule.
* **Expedition pages:** 18 today ([expedition-notes](solved/expedition-notes.md)). One page every two or three levels
  over the new count, written in order of depth. Some pages hint at a fork ahead; the last one stays the last
  fork's warning ("take the left gate. I took the right").
* **Fork hints:** for each fork the zone plan places, a page text, an inscription text, or both (see
  [denser-levels](denser-levels.draft.opus.md#route-fork-left-or-right)).
* **Riddles and names:** count the riddle gates and name gates the rebuild needs (about one each per level today) and
  add riddles (`content/riddles/*.txt`) and king names (`content/names/kings.txt`) if any would repeat.
* **Zone notes:** a journal field note on entering each zone, in the archaeologist's voice (shared with
  [duat-hours](duat-hours.draft.sonet.md) if the zones become the hours).

The texts are not placed in levels here: the rebuild places them.

## Tests

* Unit: each inscription id has a text and a glyph line, and the atlas holds them all.
* Unit: the expedition pages are numbered 1..N without gaps.
