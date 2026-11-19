# Gamepad controls on SDL3

Status: draft 2026-11-18, size M, needs nothing first.

## Idea

Play with a gamepad (Xbox, PlayStation, Steam Deck). The window and input run on SDL3 since
[sdl3-window-input](solved/sdl3-window-input.feature.md), which has `SDL_Gamepad` with a standard button layout;
GLUT had none, so the game has no pad support yet.

## Plan

* `SDL_Init` adds `SDL_INIT_GAMEPAD`; open pads on `SDL_EVENT_GAMEPAD_ADDED`, close on `_REMOVED` (hot plug). The
  events reach the game in `handleInputEvent` (`src/input/input.cpp`), next to the keys.
* A pad button is a new `InputKey::Kind::Pad` (`level/bindings.h`): a third key slot or part of the key slots, named in
  `[controls]` (`pad_a`, `pad_b`, `pad_x`, `pad_y`, `pad_lb`, `pad_rb`, `pad_lt`, `pad_rt`, `pad_start`, `pad_back`,
  `pad_dpad_up`, ...) and captured in Options > Controls like a key. Defaults: left stick / d-pad walk and climb, A
  jump, X attack, B interact, Y drag, LB / RB melee / ranged, LT sprint, RT attack (hold: the power hit), Start the
  menu, Back the inventory.
* The left stick past a dead zone is the move keys held (`setWalkHeld`); the right stick turns the look like the held
  pointer (`turnCamera`, scaled by the mouse sensitivity or its own option).
* Menus and the UI screens: the d-pad and the stick are the arrows (`NavKey`), A activates, B is Esc.
* HUD key caps (`keyCap`): the pad's button names when the last input came from the pad.

## Play-test

* Plug a pad in while the game runs: walk, climb, jump, attack (also a power hit), interact, sprint, look with the
  right stick.
* The menu, the inventory and the journal work with the d-pad, A and B; Options > Controls binds a pad button.
* Unplug and plug again: no crash, the pad works again.
