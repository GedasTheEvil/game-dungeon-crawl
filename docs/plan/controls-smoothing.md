# Controls smoothing: mouse on press, smooth look keys, quick keys by the hand, camera peek

Status: implemented 2026-11-04, waiting for the play-test (`bin/game --play tests/play/controls-smoothing.txt`). Was: draft 2026-11-04, needs nothing first. From the user: a controls analysis (QWERTY keyboard + mouse), smooth
and easy to use. Opus: input timing, camera feel and defaults in one go.

## Analysis

Defaults (`Bindings::Bindings`, `src/input/bindings.cpp`; table in [settings.md](../settings.md#controls)) checked
against the genre on keyboard + mouse: Dead Cells (WASD / arrows, Space jump, LMB / RMB weapons, F heal, R interact,
Tab map), Blasphemous (Space jump, F flask, E interact, I inventory, M map), Terraria (H quick heal, J quick mana,
1-0 hotbar, wheel cycles the hotbar).

Already good, keep: WASD + arrows to move / climb, S held to crouch, Space jump, Shift sprint held, LMB attack (held:
power hit), E interact, Q stamina skills, 1 / 2 weapons, I / M / J / K screens (paused, so reach does not matter),
Esc back, menus walked by arrows / WASD with Enter / Space / E to pick, F5 / F9 quick save / load.

Problems found:

1. **Mouse buttons act on the release.** `processMouse` (`src/input/input.cpp`): "the others run when it comes
   up". RMB jump and LMB attack (without the power hit skill) wait for the button to come up: a click's 80-150 ms of
   added lag, on the two actions where timing matters most. The keyboard Space / V act on the press. Has been so since
   the remap commit (b7e50b34), carried over from the older `if (state && ...)` code.
2. **Look keys are jerky.** Home / End / PgUp / PgDn turn `CAMERA_ROTATE_STEP` (2.5°) a key press and lean on the OS
   key repeat: one step, the repeat delay (~0.5 s), then steps at the repeat rate. Not smooth, and the speed
   depends on the desktop's repeat settings (the walk keys were moved off the repeat for the same reason,
   `stepHeldWalk`).
3. **Quick potions are out of reach.** Heal `H`, stamina `0`, antidote `=` are right-hand keys, and the right hand
   is on the mouse. The heal is the panic key: the player has to let go of the mouse in the middle of a fight. The
   genre puts it under the left hand (Dead Cells F, Blasphemous F) or on the number row next to the weapons.
4. **The camera peek never returns.** Mouse motion adds to `camera.rotM` / `rotN` (clamped ±30° / ±10°) and nothing
   brings them back. A mouse nudge while reaching for it (or a power-hit drag) leaves the view skewed until the
   player turns it back by hand. The cursor also stays visible over the game (no `glutSetCursor` anywhere).

Not problems: Interact's second key F12 and MMB are odd but harmless extras. V / Enter as keyboard attack is fine
(LMB is the main one). No change needed there.

## Plan

1. **Mouse on the press.** In `processMouse`, a bound button runs its action when it goes down, like a key: jump,
   interact and the plain attack. The attack with a power hit stays as is (down charges, up swings; a tap swings a
   plain one). Keep the held actions as they are. Guard: a click that started on another screen (closing the
   inventory with a click on its tab / outside) must not run anything on its release in the game; nothing runs on
   the release any more, so check only that the down on a screen is not routed to the game.
2. **Look keys held.** Treat the four look actions like the walk keys: held state set on press, cleared on release
   (`releaseBinding`, also `releaseWalk` when a screen opens), and each tick turn at a fixed rate (a
   `CAMERA_LOOK_KEY_SPEED` in degrees a second, about 60°/s, in `src/core/gameplay_config.h` or `input.h` next to
   the other camera constants). Key repeat no longer matters. Drop `CAMERA_ROTATE_STEP` if nothing else uses it.
3. **Quick potions on the number row.** New defaults, first key shown on the HUD (`keyCapOf`), old key kept as the
   second slot:
   * `quick_heal = 3, h`
   * `quick_stamina = 4, 0`
   * `quick_antidote = 5, equals`

   1-5 then reads as one hotbar: melee, ranged, heal, stamina, antidote. Check the HUD cards / hints that print
   these caps still fit. Update the table in `docs/settings.md` and any unit test that pins the defaults. The user's
   own `settings.ini` keeps the old values (the file is rewritten with what was read): mention in the commit that
   the three lines must be deleted (or the controls reset in Options) to get the new defaults.
4. **Camera peek eases back, cursor hidden.**
   * Hide the cursor on `Screen::Gameplay` (`GLUT_CURSOR_NONE`), show it again on every other screen and the menu
     (`GLUT_CURSOR_INHERIT` or the arrow). Where the screen changes (one place if possible, e.g. the game loop
     comparing the last screen) so no screen forgets it.
   * After about 1.5 s with no look input (mouse motion that turned the camera, or a look key held), `rotM` / `rotN`
     ease back to 0 (exponential, about 0.5 s to settle). Any look input stops the easing at once. Constants next
     to the camera ones.
   * A setting `camera_recenter = on` (graphics or controls section, Options row, `docs/settings.md`): off keeps
     today's free peek.
   * Cut-scenes and the scenario `camera` command set the rotation directly: the easing must not fight a cut-scene
     (skip while `CutScenePlaying()`), and scenario tests that set the camera then take a screenshot must not see it
     drift (check the tests that use `camera`; give the scenario runner a way to hold it, or set the delay long
     enough, whichever is simpler).

## Tests

* Unit: mouse press runs jump / attack on GLUT_DOWN, nothing on GLUT_UP (power hit keeps down-charge / up-swing).
* Unit: a look key held N ticks turns the camera N × the tick step, release stops it; clamp still holds.
* Unit: recentre easing (pure function of time since the last look input and the current angle, so it tests
  without the game).
* Unit: new defaults (`Bindings` default table).
* Play-test script `tests/play/controls-smoothing.txt`: a level with a monster and potions in the inventory, to feel
  the click timing, the look keys, the 3-5 drinks and the camera easing back.

## Done

* Mouse: every bound button acts on the down, its up only lets go (`processMouse`). Scenario
  `tests/scenarios/controls_smoothing.txt`: LMB down swings at once, the up runs nothing, a down on the inventory
  then the screen closed runs nothing.
* Look keys held, `CameraLook` (`src/input/camera_look.h`, pure, `tests/unit/camera_look_test.cpp`):
  `CAMERA_LOOK_KEY_SPEED` 60°/s, `stepCameraLook` once a tick; `CAMERA_ROTATE_STEP` dropped.
* Defaults: heal `3, h`, stamina `4, 0`, antidote `5, equals` (settings test, docs/settings.md).
* Cursor hidden over the game while alive and not won (`followCursor`, each Update, set on change only).
* Recentre after 1.5 s idle, time constant 160 ms (settled ~0.5 s); skipped in cut-scenes (the tick returns before
  it). Setting `camera_recenter` in `[graphics]`, Options > Display > Camera recentre (Display rows 5.04 high to fit
  ten). Off in scenarios, so `camera` + screenshot stays put.
