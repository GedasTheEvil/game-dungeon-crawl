# Replace the Bes statue with Thoth

Status: draft 2026-10-07, from the user.

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
  Thoth four times as common as any other prop. Pick "a Thoth" as one slot, then the variant by a second hash (or
  per-prop weights, which [decor-by-depth.draft.md](decor-by-depth.draft.md) needs anyway).
* Torches: `Dungeon::scatterTorches` skips cells with Bes because his plumes reach the wall torch. Keep the rule for
  the variants whose crown reaches that high (the standing ones, likely).
* Tests: `tests/scenarios/statues.txt` (screenshot `osiris_bes`), `tests/levels/statues29063.txt` (Bes at col 10):
  show all four variants.
* Docs: [decor-by-depth.draft.md](decor-by-depth.draft.md) puts Bes in the tomb tier: swap in Thoth.
  [solved/statue-and-mummy-decorations.md](solved/statue-and-mummy-decorations.md) explains why Bes was picked:
  leave it as history.

## Open

* Heights per variant (the torch rule).
* Variant weights: equal, or the classic forms (standing ibis, seated baboon) more common.
