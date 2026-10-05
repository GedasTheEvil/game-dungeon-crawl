# Sprint motion effect

Status: draft 2026-10-05.

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
* Optional extras: dust puffs at the feet, a faster footstep sound and a slight camera lag behind the player.

## What to settle

* Ease in and out over about 150-250 ms. No hard on/off, also when stamina runs out mid-sprint.
* Strength scales with real movement: sprint held while standing still or blocked by a wall shows nothing.
* An Options toggle (motion sickness): "Motion effects" on/off, also for the FOV kick.
* Toon mode: the blur goes before the ink lines or after them? Lines streaking may look wrong.
* HUD and status lines are drawn after the scene pass, so they stay sharp.
* Without FBOs (old driver): FOV kick only.
* Scenario: sprint along a corridor, screenshots at rest, mid-sprint and after the stop.
