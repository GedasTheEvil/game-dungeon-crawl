# Cinematic effects

Status: draft 2026-10-10, ready (needs nothing first). From the user: short cut-scenes for remote events, e.g. a
lever that opens a gate elsewhere: the camera moves to the gate, shows it opening, and comes back. The view looks
distinct while it plays.

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
