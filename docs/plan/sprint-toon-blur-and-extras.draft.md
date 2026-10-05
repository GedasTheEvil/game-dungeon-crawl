# Sprint blur in toon mode, sprint extras

Status: draft 2026-10-05. Split off [sprint-motion-effect.md](sprint-motion-effect.md).

## Idea

The sprint motion effect (FOV kick, radial blur, vignette) ships with the radial blur in normal mode only. Toon mode
gets the FOV kick (and vignette) without the blur, until this is settled.

## What to settle

* Toon mode: the radial blur goes before the ink lines or after them? Blurring after the lines streaks them, which
  may look wrong; before the lines keeps them crisp. Try both, compare screenshots mid-sprint.
* Extras, decide together with the above:
  * dust puffs at the feet,
  * a faster footstep sound,
  * a slight camera lag behind the player.
* All of them under the "Motion effects" option, except perhaps the footstep sound.
