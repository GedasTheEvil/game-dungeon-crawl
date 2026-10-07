# Replace the Bes statue with Thoth

Status: draft 2026-10-07, from the user.

The user does not like the Bes decoration. Remove it and add a broken Thoth statue in its place.

## Idea

* Thoth, god of writing and wisdom: ibis-headed man (long curved beak), striding or seated, holding a scribe's
  palette and reed pen, a crescent moon and disc on the head. The alternative form, a seated baboon with the moon
  disc, is a squat silhouette like Bes had.
* Broken like the other statues: chips, a snapped beak or arm, pieces on the floor in front.
* Fits the scribe theme: scrolls prop (`scrolls`) and glyph decals already exist.

## Where it fits

* Model: `tools/blender/models/decor.py`: replace `build_bes` (and its helper) with `build_thoth`, the `PROPS`
  entry and its scale (`"bes": 1.25`). Old `models/decorations/decor_bes.md3` and
  `textures/decorations/decor_bes.png` get deleted. See [../remodeling.md](../remodeling.md) (the `bes` line).
* Engine: `src/world/decor.h`: `DECOR_BES` (13) becomes `DECOR_THOTH`, name in `DECOR_NAMES`.
  `Dungeon::scatterTorches` (`src/world/dungeon_decor.cpp`) skips torches over Bes because the plumes reach the
  torch: keep the rule only if Thoth's crown is as tall.
* Tests: `tests/scenarios/statues.txt` (screenshot `osiris_bes`), `tests/levels/statues29063.txt` (Bes at col 10).
* Docs: [decor-by-depth.draft.md](decor-by-depth.draft.md) puts Bes in the tomb tier: swap in Thoth.
  [solved/statue-and-mummy-decorations.md](solved/statue-and-mummy-decorations.md) explains why Bes was picked:
  leave it as history.

## Open

* Ibis-headed man or baboon form.
* Standing or seated; how tall (the torch rule).
