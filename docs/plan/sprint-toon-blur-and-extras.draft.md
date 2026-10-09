# Sprint blur in toon mode, sprint extras

Status: draft 2026-10-05, refined 2026-10-09 (decided, not implemented). Split off [sprint-motion-effect.md](solved/sprint-motion-effect.md).

## Idea

The sprint motion effect (FOV kick, radial blur, vignette) ships with the radial blur in normal mode only. Toon mode
gets the FOV kick (and vignette) without the blur, until this is settled.

## Decided (2026-10-09)

* **Toon mode blur:** try both orders, the radial blur before the ink lines and after them. A scenario takes
  mid-sprint screenshots of each (`tests/scenarios/`), the user picks one; the other goes.
* **Dust puffs at the feet:** a small puff on each step while sprinting on a dry floor, none in water. Under the
  "Motion effects" option.
* Dropped: a faster footstep sound, the camera lag.
