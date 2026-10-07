# Replace the Bes statue with Thoth

Status: implemented 2026-10-07, not play tested. Draft 2026-10-07, from the user.

The user does not like the Bes decoration. Remove it and add broken Thoth statues in its place, in several variants.

## Idea

Thoth, god of writing and wisdom. Four props, each broken like the other statues (chips, a snapped beak, arm or
crown, pieces on the floor in front):

| Prop | Form |
|---|---|
| `thoth_ibis_standing` | ibis-headed man striding, scribe's palette and reed pen in hand |
| `thoth_ibis_seated` | ibis-headed man on a block throne, palette on the knees |
| `thoth_baboon_seated` | baboon squatting on a plinth, hands on the knees (the classic form) |
| `thoth_baboon_standing` | baboon upright, forepaws raised in adoration (as at sunrise) |

* Every variant wears the crescent moon and disc (it can be the broken-off piece on the floor).
* Fits the scribe theme: the `scrolls` prop and the glyph decals already exist.

## Where it fits

* Model: `tools/blender/models/decor.py`: replace `build_bes` (and its helper) with one `build_thoth_*` per variant,
  sharing the ibis head, the baboon body and the moon crown. `PROPS` entries and scales (Bes was `"bes": 1.25`).
  Old `models/decorations/decor_bes.md3` and `textures/decorations/decor_bes.png` get deleted. See
  [../remodeling.md](../remodeling.md) (the `bes` line).
* Engine: `src/world/decor.h`: `DECOR_BES` (13) goes; four new entries in `DECOR_NAMES`, `DECOR_COUNT`,
  `DECOR_SCATTERED`, `DECOR_COFFIN` (must stay last) shift; `DECOR_JITTER` in `src/world/dungeon_decor.cpp` gets
  four values.
* Frequency: `Dungeon::scatterDecorations` picks uniformly over `DECOR_SCATTERED`, so four variants would make
  Thoth four times as common as any other prop. Pick "a Thoth" as one slot, then the variant at random (equal odds)
  by a second hash. Thoth only turns up where his tier allows (tomb tier in
  [decor-by-depth.md](decor-by-depth.md)); all four variants share that tier.
* Height: up to Osiris' size (`osiris_statue`, scale 1.4; upright with the atef crown about 0.4 tile). The wall
  torch bracket sits at 0.40 (`TORCH_BASE`), so a tall Thoth reaches it: `Dungeon::scatterTorches` keeps the Bes
  rule (no torch on the cell) for all four variants.
* Tests: `tests/scenarios/statues.txt` (screenshot `osiris_bes`), `tests/levels/statues29063.txt` (Bes at col 10):
  show all four variants.
* Docs: [decor-by-depth.md](decor-by-depth.md) puts Bes in the tomb tier: swap in Thoth.
  [solved/statue-and-mummy-decorations.md](solved/statue-and-mummy-decorations.md) explains why Bes was picked:
  leave it as history.

## Decided (2026-10-07, the user)

* Four variants, picked at random with equal odds, all in Thoth's tier.
* Size: as tall as Osiris at most.

## Done (2026-10-07)

* `tools/blender/models/decor.py`: `build_bes` gone; `moon_crown`, `ibis_head`, `ibis_figure`, `baboon_figure` and one
  `build_thoth_*` per variant, scale 1.4, up to 0.36 tile tall (Osiris upright ~0.4). The ibis figures limestone with a
  blue-striped wig, turned a little to the side so the beak shows; the baboons dark sandstone with a long muzzle and a
  mane cape. Broken: the striding ibis's beak (the tip on the floor) and crown, a corner of the throne, the seated
  baboon's crown, the standing baboon's right forearm.
* `src/world/decor.h`: `DECOR_THOTH` (13) and `DECOR_THOTH_VARIANTS` (4), `isThoth`; `DECOR_COUNT` 19, the coffin
  last. `Dungeon::scatterDecorations` picks among `DECOR_PICKS` (Thoth's variants as one), then the variant from the
  cell's hash. No wall torch over any Thoth.
* `decor_bes` model and texture deleted.
* Scenario command `prop COL NAME` (put a prop on the player's row); `tests/scenarios/statues.txt` shows a scattered
  Thoth at col 10 and all four side by side.
* The tier (tomb, from level 8) comes with [decor-by-depth.md](decor-by-depth.md).
