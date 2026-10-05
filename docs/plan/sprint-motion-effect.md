# Sprint motion effect

Status: implemented 2026-10-05, awaiting verification. Decided: FOV kick, radial blur + vignette; "Motion effects"
on/off in Options; toon blur and extras later.

Done: `src/graphics/motion_fx.cpp` (`MotionFx`). Strength eases over 200 ms by the game clock, on while
`PlayerStats::IsSprinting()` (shift held and a real step) and the option is on. FOV 45° + up to 6°. Radial blur
(12 samples, reach grows away from the player's chest, projected each frame) in a `RenderTarget` post pass, normal
mode only. Vignette as a blended overlay, also in toon mode. No FBO: FOV kick and vignette; no shaders: FOV kick.
Options > Display tab with the toggle, saved in `saves/settings.txt` (`src/state/settings.cpp`, not in scenarios).
Fixed on the way: the HUD tested against stale window depth after an offscreen scene (toon mode showed a black box
over the HUD); `Ink::end()` and `MotionFx::end()` clear it. Scenario `tests/scenarios/sprint_motion.txt`.

## Idea

Sprinting (3x walk speed, `PlayerStats::SprintMoveMultiplier()`) looks the same as walking, only faster. It
should feel fast: a motion blur style effect while the player sprints.

## Notes

The camera is a fixed chase camera behind the player (`drawGameplay()` in `src/graphics/draw.cpp`). A full-screen
blur would blur the player too, the one thing the eye follows. Better options:

* **Radial blur** from the screen centre: the edges streak, the player stays sharp. A post pass on the
  scene's offscreen target. `Ink` (`src/graphics/ink.cpp`) already draws the scene into an FBO and copies it to the
  screen with a shader, but only in toon mode. A shared scene target (or `RenderTarget`, from the journal work)
  would serve both.
* **FOV kick**: the 45° of `gluPerspective` goes up a few degrees while sprinting and eases back after. Cheap, no
  FBO, and gives most of the speed feel by itself. Could be step 1.
* **Vignette** dimming the edges, which hides the blur's seams too.
* Extras (dust puffs, footstep sound, camera lag): moved to
  [sprint-toon-blur-and-extras.draft.md](sprint-toon-blur-and-extras.draft.md).

## What to settle

* Ease in and out over about 150-250 ms. No hard on/off, also when stamina runs out mid-sprint.
* Strength scales with real movement: sprint held while standing still or blocked by a wall shows nothing. Same
  "actually moving" signal as [sprint-drain-standing-still.md](solved/sprint-drain-standing-still.md).
* An Options toggle (motion sickness): "Motion effects" on/off, also for the FOV kick.
* Toon mode: no blur for now (FOV kick and vignette only). Blur order vs the ink lines: moved to
  [sprint-toon-blur-and-extras.draft.md](sprint-toon-blur-and-extras.draft.md).
* HUD and status lines are drawn after the scene pass, so they stay sharp.
* Without FBOs (old driver): FOV kick only.
* Scenario: sprint along a corridor, screenshots at rest, mid-sprint and after the stop.
