# Gamepad controls on SDL3

Status: implemented 2026-11-27, waiting for the user's play-test. Was: draft 2026-11-18, size M, needs nothing first.

## Idea

Play with a gamepad (Xbox, PlayStation, Steam Deck). The window and input run on SDL3 since
[sdl3-window-input](solved/sdl3-window-input.feature.md), which has `SDL_Gamepad` with a standard button layout;
GLUT had none, so the game has no pad support yet.

## Plan

* `SDL_Init` adds `SDL_INIT_GAMEPAD`; open pads on `SDL_EVENT_GAMEPAD_ADDED`, close on `_REMOVED` (hot plug). The
  events reach the game in `handleInputEvent` (`src/input/input.cpp`), next to the keys.
* A pad button is a new `InputKey::Kind::Pad` (`level/input/bindings.h`): a third key slot or part of the key slots, named in
  `[controls]` (`pad_a`, `pad_b`, `pad_x`, `pad_y`, `pad_lb`, `pad_rb`, `pad_lt`, `pad_rt`, `pad_start`, `pad_back`,
  `pad_dpad_up`, ...) and captured in Options > Controls like a key. Defaults: left stick / d-pad walk and climb, A
  jump, X attack, B interact, Y drag, LB / RB melee / ranged, LT sprint, RT attack (hold: the power hit), Start the
  menu, Back the inventory.
* The left stick past a dead zone is the move keys held (`setWalkHeld`); the right stick turns the look like the held
  pointer (`turnCamera`, scaled by the mouse sensitivity or its own option).
* Menus and the UI screens: the d-pad and the stick are the arrows (`NavKey`), A activates, B is Esc.
* HUD key caps (`keyCap`): the pad's button names when the last input came from the pad. Names follow the pad
  family (`SDL_GetGamepadType` / `SDL_GetGamepadButtonLabel`): A B X Y on Xbox-style pads, ✕ ○ □ △ (Share, Options,
  L1 R1 L2 R2) on PlayStation pads. The bindings stay positional (`pad_a` is the bottom face button on every pad).
* Test pad: a DualShock 4 (USB or Bluetooth). Without hidraw access SDL falls back to evdev; buttons and sticks
  still work.

## Implementation

What was built, and where it left the plan above:

* Each action has one pad button, a 4th slot (`PAD_SLOT`, the Pad column in Options > Controls); saved in its own
  `[gamepad]` section (`pad_a`, ...), so a `settings.ini` from before keeps its `[controls]` and gets the default pad
  buttons. `[gamepad]` also holds `look_speed`, `dead_zone`, `invert_y` (Options > Look, the Mouse tab renamed).
* Fixed, not bindable (like mouse look): the left stick walks and climbs, the right stick looks (a peek the camera
  recentre eases back, like the look keys'), Start is Esc. On screens the d-pad and the left stick are the arrows, A
  Enter, B Esc; a button bound to a screen (Back: inventory) switches or closes it there too.
* Defaults differ from the plan: the d-pad is the potions (up heal, right stamina, left antidote) and the map (down),
  since the left stick walks; X attacks (hold: the power hit), RT is the stamina skills toggle, the stick clicks open
  the journal (L3) and the Heka screen (R3).
* Triggers count as buttons past half way (`pad_lt`, `pad_rt`); sticks have a dead zone (default 25 %) with a little
  hysteresis.
* Key caps: `GameState::caps` follows the last input (keyboard / Xbox / PlayStation, from `SDL_GetGamepadType`); the
  HUD and the journal hint show the pad's names. The fonts got glyphs for ✕ ○ □ △ and the d-pad arrows (control
  characters 1-8, `render/glyphs.h`; the sheets' spare cells, the down / left / right arrows the up one turned). The
  user asked for the face symbols in their own colours: ✕ blue, ○ red, □ pink, △ green (`padGlyphTint`, `ui::text`'s
  colour pass).
* Scenario commands `pad` and `stick` (docs/testing.md); `tests/scenarios/sdl3-gamepad*.txt`.
* Left out: a held d-pad or stick does not repeat in the menus (one step a push); How to Play and the first hints
  still name the keyboard's keys.
* Checked on a DualShock 4 over USB: SDL opens it through hidraw as `ps4`; the game logs `Gamepad: PS4 Controller`.

## Play-test

`bin/game --level 1`

* Plug a pad in while the game runs: walk, climb, jump, attack (also a power hit), interact, sprint, look with the
  right stick.
* The menu, the inventory and the journal work with the d-pad, A and B; Options > Controls binds a pad button.
* Unplug and plug again: no crash, the pad works again.
* With the DualShock the HUD names its buttons (L1 R1, D▲, the interact prompt ○); after a key press the keys again.
