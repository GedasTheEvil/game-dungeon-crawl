# Cinematic effects

Status: solved 2026-10-18 (play-tested by the user). Play-test: `bin/game --play tests/play/cinematic-effects.txt`
(the lever by the start opens three gates: one on the screen, two far ones the cut-scene shows). From the user: short
cut-scenes for remote events, e.g. a lever that opens a gate elsewhere: the camera moves to the gate, shows it opening,
and comes back. The view looks distinct while it plays.

## When

* A lever opens a gate that is off-screen (`Dungeon::PullLever` → `openGates`, `src/world/dungeon_mechanisms.cpp`).
  Several gates of that colour: show the nearest off-screen one (or each in turn, max 2).
* A boss dies and its boss gates open (off-screen ones).
* Optional (implementer's choice): a key picked up, first sight of a boss (a short intro pan), a riddle gate opening.
* On-screen gates: no cut-scene, as today.

## How

* The game pauses (monsters, poison, timers); only the gate animation and its sound run.
* Camera: ease out to the gate (~0.6 s), hold while it opens (~1 s), ease back (~0.6 s). The view follows the same
  rules as the normal camera (render distance, explored cells; the gate's surroundings are drawn even if unexplored,
  or only the gate cell: implementer's choice).
* Look: letterbox bars (top and bottom, ~8% each, slide in), slight desaturation or sepia tint and a vignette (reuse
  `motion_fx`). HUD hidden.
* Skip: any key or click jumps back at once.
* Option: "Cut-scenes" on/off in the options menu (`docs/ui.md`), on by default; off = today's behaviour.
* Scenario scripts: cut-scenes off by default in tests, or a command to skip them, so timings do not change.

## Tests

* Unit / sim: pulling a lever with an off-screen gate starts a cut-scene; the world does not advance during it; skip
  ends it.
* Scenario with screenshots: mid-pan with letterbox, back at the player.

## Implemented (2026-10-18)

* `src/world/dungeon_cutscene.cpp`: `Dungeon::openGates` (a lever, a boss's death) hands the gates it opened to
  `startCutScene`: the ones outside the gameplay window (`ViewWindow`), the nearest 2, nearest first. Each gate's motion
  starts as the camera comes to it. The camera eases (smoothstep) to each gate in `CUTSCENE_PAN_MS` (600), holds
  `CUTSCENE_HOLD_MS` (the gate's 1.2 s and 0.2 s more), eases back. `Dungeon::Update` only moves the gates meanwhile;
  the game loop skips the walk, the attack, poison, stamina, regeneration, play time and every pose. A save is not
  possible during one (the menu key skips it); a load clears it.
* The camera: `view()` and the level frame follow `CutShot` (the camera's place from the player), so the gate's
  surroundings are drawn, explored or not; `draw.cpp` moves the player and its lights by the same amount and puts a
  player-like light at the camera, so the far gate is lit.
* The look (`motion_fx`): sepia tint (on the blur's offscreen pass; not in toon mode, as the blur), its vignette and
  letterbox bars of 8% each, all in and out over `CUTSCENE_BARS_MS`. The HUD, the interact prompt and the boss bar are
  hidden; the status line stays as a caption ("Somewhere a Carnelian gate grinds open").
* Skip: any key, special key or click (`input.cpp`), also Esc (it does not open the menu then); the skipping click's
  release does nothing. Key repeat is off while one plays, so a walk key held from before does not skip it. The far
  gates start opening at the skip.
* Option: Options > Display > Cut-scenes (`cut_scenes` in `saves/settings.ini`), on by default. The Display rows got
  a little closer (`OPTION_H` 7) to fit the seventh. Scenarios run with it off; the `cutscenes on|off` command and the
  `expect cutscene` field.
* Not done (the optional ones): a key picked up, a boss's first sight, a riddle gate opening. The gate sound still
  plays at the pull, not as the camera arrives.
* Tests: `tests/unit/cutscene_test.cpp` (the stops and their order, the world standing still, the end, skip, off, an
  on-screen gate, a boss's death; levels `tests/levels/cutscene`, `cutscene_boss`), scenario
  `tests/scenarios/solved/cinematic_effects.txt` (mid-pan, each gate, back, skipped, toon).
