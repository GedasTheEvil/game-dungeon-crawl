# Duat wall paintings: the night barque, Apep bound, the scales, Nut

Status: solved 2026-11-15 (play-tested by the user). Was: implemented 2026-11-15, waiting for the user's play-test. Was: draft 2026-11-13, size L, needs nothing first. From the lore coverage audit of 2026-11-13.

## Idea

The player walks the twelve hours of Ra's night journey, and the walls never show it. The game's tombs and temples
are the places that were painted with exactly that (the Amduat in the royal tombs of the Valley of the Kings, the
weighing of the heart on every Book of the Dead papyrus), yet:

* The wall decals (`DECAL_DEFS`, `src/level/decor.h`, drawn by `tools/textures/decals.py`) are 16, all generic:
  cracks, vines, roots, seepage, a glyph column, a glyph row, a cartouche, the eye of Horus, the winged sun, papyrus,
  grass, creeper, moss. None is a scene. The atlas is full (`DECAL_ATLAS_GRID` 4, `DECAL_COUNT` 16).
* The scenes are only told, never shown: the boat of Ra (20 text hits for "boat|barque", no prop, no decal), Apep's
  binding (ids 17, 18), the weighing (ids 131, 134), Nut (3 hits). The props have Osiris, Thoth and the jackal as statues
  (`DECOR_NAMES`), no painted scene.
* Ra's night form, the ram-headed sun in his shrine on the barque, is in no text and no picture (0 hits for ram-headed
  / "flesh of Ra"; see [hour-gate-stelae](../hour-gate-stelae.feature.md) for the texts).

## Plan

Five to six painted wall decals in the style of the existing glyph panels (flat paint, ink outline, the `PAINT`
colours, weathered with `weather()`), each a scene readable at a cell's size:

| Decal | What it shows | Zones (factor > 1 in `ZONE_LOOKS`, `src/level/zone_look.h`) |
|---|---|---|
| night barque | the boat with a ram-headed figure in a shrine, a coiled serpent (Mehen) over the shrine, towing figures | all temple zones from the priests' quarter on, most in the cisterns (hour 7) |
| Apep bound | a long serpent pinned by knives, a figure with a knife (or the cat of Ra) | serpent temple, cisterns |
| the weighing | a balance, heart and feather, a jackal steadying it, an ibis-headed scribe, a crouching beast (Ammit) | necropolis, Hall of Judgment |
| Nut | the sky goddess arched over a wall, stars on her body, a small sun at her mouth and her hips | Hall of Judgment, the Lake of Fire |
| baboons adoring | two or three seated baboons with raised hands before a sun disc | Lake of Fire (Thoth's baboons, Babi's shore), Hall of Judgment |
| Aker (optional) | two lions back to back, the sun between | Land of Sokar, Seth's desert |

* Atlas: grow the grid to 8 x 4 or 8 x 8 (`DECAL_ATLAS_GRID`, `atlasCell`), or a second atlas; keep the cell order in
  step with `DECAL_DEFS` (the script's docstring says so). Big scenes may want a wider anchor than one cell: check how
  `DecalDef`'s size is used before drawing at 1x1.
* Placement: by the zone factors (generated dressing) and by hand on the campaign levels where a scene suits a text
  next to it (the weighing beside inscription 131 on lvl52, Apep bound beside id 18 on lvl27, the barque by the hour
  stela if that draft is done). The editor paints decals ([placed-decor](placed-decor.md)); levels are the
  source of truth, never regenerate them.
* Comic look (`F1`): decals get the ink outline like the rest; check that thin strokes survive.
* Glossary: one row per new decal name if the glossary lists decals (grep first).

## Open questions

* Hand-drawn quality: procedural PIL painting of a figure scene is hard. Simplified silhouettes in the Egyptian
  register style (profile figures, flat colour) are fine; if a scene does not read at game distance, drop it.
* How many per level, so they stay special: a zone factor low enough for one or two a level.

## Done

* Six scenes, decals 16-21 (`DECAL_SCENE_FIRST`, `src/level/decor.h`), drawn in `tools/textures/decals.py` (Duat
  scenes section): night barque (the ram-headed sun with the disc in his shrine, Mehen over it, two men and a jackal
  hauling), Apep bound (four knives, the cat of Ra with a knife), the weighing (heart and feather, Anubis, Thoth facing
  the scale, Ammit crouched), Nut (arched, stars, the evening sun at her mouth, the morning sun at her hips), baboons
  (three, praising the sun), Aker (two lions back to back, the sun on the horizon). Each on a plaster band with the
  ground line and the block border; free anchor, size 0.8.
* Atlas grown to 8 x 8 (`DECAL_ATLAS_GRID`; 22 of 64 cells used), 2048 px.
* Rarity (open question): a scene comes from its own roll, `DECAL_SCENE_PERMILLE` (8) of the decal cells, at most
  `DECAL_SCENES_PER_LEVEL` (2) a level from the scatter; the other decals keep their rolls (golden diffs: only scene
  lines). Zone factors in `ZONE_LOOKS`: barque priests' quarter, serpent temple, cisterns (3), Lake of Fire,
  necropolis, Hall of Judgment; Apep serpent temple (3), cisterns; weighing necropolis, Hall of Judgment (3); Nut and
  baboons Lake of Fire, Hall of Judgment; Aker Seth's desert, Land of Sokar (3). 26 of the 41 temple-tier levels get
  one or two.
* By hand (decal layer): the weighing beside inscription 131 (lvl52), Apep bound beside 18 (lvl27), the barque beside
  the seventh hour's stela (lvl28), the baboons beside 117 "The baboons of the lake" (lvl42).
* The checker's decal letters skip 't' (a torch alone): `decalLetter`.
* Comic look: the ink outlines survive (scenario `duat_wall_paintings`, `002_weighing_lvl52_toon`).
* Glossary: "Duat scene".

## Play-test

* `bin/game --play tests/play/duat-wall-paintings.txt` (lvl52, by inscription 131): the weighing is painted beside
  it. F1: the comic look keeps the figures.
* `bin/game --level 28`: the night barque beside the seventh hour's stela, at the start.
* `bin/game --level 42`: the baboons beside the inscription "The baboons of the lake" (top floor, right of middle).
* `bin/game --level 44` (Nut, the baboons) and `--level 35` (Aker): scattered scenes, rare: one or two a level.
