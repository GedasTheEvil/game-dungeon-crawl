# Duat wall paintings: the night barque, Apep bound, the scales, Nut

Status: draft 2026-11-13, size L, needs nothing first. From the lore coverage audit of 2026-11-13.

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
  / "flesh of Ra"; see [hour-gate-stelae](hour-gate-stelae.feature.draft.sonet.md) for the texts).

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
  stela if that draft is done). The editor paints decals ([placed-decor](solved/placed-decor.md)); levels are the
  source of truth, never regenerate them.
* Comic look (`F1`): decals get the ink outline like the rest; check that thin strokes survive.
* Glossary: one row per new decal name if the glossary lists decals (grep first).

## Open questions

* Hand-drawn quality: procedural PIL painting of a figure scene is hard. Simplified silhouettes in the Egyptian
  register style (profile figures, flat colour) are fine; if a scene does not read at game distance, drop it.
* How many per level, so they stay special: a zone factor low enough for one or two a level.

## Play-test

* `bin/game --level 52`: the weighing painted beside its inscription.
* `bin/game --level 30`, `--level 44`: the barque and the baboons on the walls of their zones; F1 for the comic look.
