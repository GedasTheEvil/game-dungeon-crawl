# Camera recentre undoes the mouse look

Status: draft 2026-11-08, size S, needs nothing first.

## Repro

1. `bin/game --level 1`, default settings (Options > Mouse, "Camera recentre" on: `[graphics] camera_recenter`,
   default true in `src/level/settings_ini.h`).
2. Move the mouse to look left, then stop.

## Expected

The view stays where the mouse turned it. The mouse is where the player looks; it moves back only when the player
moves the mouse back.

## Actual

After `CAMERA_RECENTER_DELAY_MS` (1.5 s) without look input, `CameraLook::Step` (`src/level/camera_look.h`) eases
`rotM` / `rotN` back to 0, for a mouse look too (`CameraLook::Looked` only restarts the delay). Added by
[controls-smoothing](solved/controls-smoothing.md), switch in [settings-extras](solved/settings-extras.feature.md).

## Notes

* Suggested fix: the recentre applies to a look-key peek only. A mouse turn cancels the pending recentre for good
  (until a look key is used again), so a view the mouse set stays. Keep the option for the keys; reword its hint
  ("A look-key peek eases back when left alone").
* Implementer's choice if that feels wrong in play: default the option to off instead. Say which in the plan.
* Interplay with [mouse-leaves-window](mouse-leaves-window.bug.draft.opus.md): independent, either can go first.

## Fixed when

* A unit test that names the plan: after a mouse look and a long idle, `CameraLook::Step` keeps the angles; after a
  look-key peek and the delay, it still eases back (recentre on).

## Play-test

* `bin/game --level 1`: look left with the mouse, wait 3 s: the view stays.
* Peek with a look key, release, wait: the view eases back (Camera recentre on).
