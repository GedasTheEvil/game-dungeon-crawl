# Controls help and first-level hints

Status: solved 2026-11-08 (play-tested by the user). Was: implemented 2026-11-08, waiting for the user's play-test. Was: draft 2026-10-30, size L, needs nothing first. From the project audit of 2026-10-30.

Opus: a new menu sub-screen and in-level teaching.

## Problems

* No tutorial, no help or controls screen. The in-game menu has no Controls / Help entry. Movement, jump, attack,
  sprint, the power-hit charge and the quick keys are never taught. The journal prologue is story only.
* The readme control table leaves out F5 quick save and F9 quick load (only in `docs/settings.md`).
* F3 (show hitboxes) is a debug toggle open to players, listed as a fixed key in Options (`input.cpp`, `menu.cpp`).

## Plan

* A "Controls" page reachable from the main menu and the in-game menu (and maybe a key): the current bindings,
  read from `bindings.cpp`, grouped (move, fight, items, screens, saves), in the screen look.
* First levels: short context hints the first time something is possible, shown once per run, using the bound
  key's name: jump at the first gap, climb at the first ladder, interact at the first chest / lever, attack at the
  first monster, sprint, the power hit (once learned), the quick potion keys at the first potion pickup. Use the
  interact prompt or the status line; a setting to turn hints off.
* F3 only with a `--debug` command-line flag (or a dev build); drop it from the Options list.
* Readme: add F5 / F9.

## Done

* **How to Play** page (main menu, in-game menu): the keys as bound now in five groups (Move, Fight, Items, Screens,
  Saves) from `controlsHelp` (`src/level/controls_help.*`), drawn by `MainMenu::DrawHelp` (`src/ui/menu_help.cpp`).
  Named "How to Play", not "Controls": Options already has a Controls tab (the rebinding table); the page links to it
  ("Change keys"). No extra key opens it (F1-F11 are taken or fixed).
* **First hints**: a light blue status line (`StatusTone::Hint`, 6 s) the first time in a campaign run that the player
  faces a gap or a one-cell ledge (jump), stands at a ladder (climb), a monster comes for them (attack), walked 4 s
  without sprinting (sprint), has the power hit learnt, holds a healing / stamina potion or an antidote (quick keys).
  Each names the bound key (`hintText`), shows once a run (`RunStats::hints`, a `HINTS` line in the save), at most one
  per 7 s; a thing the player already did (jumped, climbed, attacked, sprinted, charged) gets no hint. Interact needs
  none: the interact prompt ("[E] Open") already teaches it at the first chest or lever. Switch: "First hints" on the
  How to Play page, `[gameplay] hints` in settings.ini. Triggers in `src/state/first_hints.cpp`.
* **F3** (hitboxes) works only with `bin/game --debug`; dropped from the Options fixed-key line and the docs.
* **Readme**: F5 / F9 were already in the table; added a line on How to Play and the hints.

## Play-test

`bin/game --play tests/play/controls-help.txt` (level 1, hints on as in a New Game run).

* Walk right: a light blue line names the keys at the first gap / ladder / monster / after walking a while ("Jump:
  Space or RMB"); each comes once, a few seconds apart.
* Esc > How to Play: the keys in five groups; rebind one in Change keys (Options > Controls) and the page follows.
* On the How to Play page, switch "First hints" off: no more hints. F3 does nothing (only with `--debug`).
