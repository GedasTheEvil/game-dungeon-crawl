# Controls help and first-level hints

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

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
