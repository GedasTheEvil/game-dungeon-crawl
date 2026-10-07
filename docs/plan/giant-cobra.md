# Giant cobra

Status: implemented 2026-10-06 (not play tested). Draft 2026-10-06. Asked for by the user beside the [cobra](cobra.md). Needs the cobra (done).

The cobra's giant kin, like the giant rat, bat and scarab: the same model and clips with its own texture, bigger, stronger.

* Model: `monsters/cobra` (all clips), texture `monsters/cobra_giant` (black-necked: near black with a pale throat
  band and amber eyes), baked by `tools/blender/models/cobra.py` on the same UVs (`COBRA_TEX=giant` for the review
  renders), as the rat / bat / scarab scripts do.
* Behaviour: as the cobra (coiled, rears up, spits, bites), a longer spit.
* Poison: medium, like the cobra (strong stays the scorpion queen's).
* Swimmer like the cobra.

## Numbers (starting values)

| | Cobra | Giant cobra |
|---|---|---|
| Speed | 6 | 8 |
| HP | 35 | 110 |
| Bite | 6 / 1100 ms | 16 / 1300 ms |
| Spit | 2, 2.5 tiles, 3.5 s | 5, 3 tiles, 3 s |
| XP | 1200 | 2600 |
| Scale | 24 | 36 |

Resistances as the cobra. Checker threat 5. Glyph `G` is taken (green gate): use `Q`.

## Placement

Levels 21-30 of the [longer campaign](longer-campaign.md); the cobra gives way to it after Apep (lvl20), as
the weak monsters give way to their giant kin.

## Done (2026-10-06)

* `MonsterGiantCobra` (18, glyph `Q`), texture `cobra_giant.png`, numbers as above. Placed in levels 21-30
  (`./levelcheck`, 2026-10-07: 3 to 8 a level).
