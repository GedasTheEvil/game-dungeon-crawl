# Options: "Reset to defaults" on the Display and Sound tabs

Status: Won't do 2026-11-04: only the Controls reset was wanted, and it already exists. Was: draft 2026-11-04, needs nothing first. From the user: a reset to defaults button in the settings.

Sonnet: copies the Controls tab's existing button to the other two tabs.

## Now

Options has three tabs (`TABS`, `src/ui/menu.cpp`): Controls, Display, Sound. Only Controls has a
"Reset to defaults" button (`resetButton()`, target `RESET_CONTROLS`). Clicking it sets
`Game().settings.controls = Bindings{}`, calls `ApplySettings(true)` and shows the toast "Controls reset to the
defaults". Display and Sound have no reset.

## Plan

* Same button, same look ("Reset to defaults", stone tile, under the last option row) on the Display and Sound tabs.
  It resets only the tab it is on.
* Display: reset `Settings::Display` and `Settings::Graphics` (the rows the Display tab shows) to their
  default-constructed values, **except the window size and fullscreen**: keep those as they are. Those defaults
  depend on the machine, and a reset that drops a fullscreen player into an 800x500 window is a surprise. Reset by
  assigning a default-constructed struct and copying the kept fields back, so rows added later (plans
  [settings-extras](../settings-extras.draft.opus.md), [controls-smoothing](../controls-smoothing.md)) are
  covered without touching the reset.
* Sound: `Settings::Sound{}`.
* Then `ApplySettings(true)` (saves the file, applies the volumes / toon / ...), and a toast: "Display reset to the
  defaults" / "Sound reset to the defaults".
* Reachable by the keyboard like the Controls button ([keyboard-menus](../solved/keyboard-menus.md)): the focus walks
  onto it after the last row, and Enter / Space / interact presses it.
* Mouse hover, press and the hint line work as on the Controls button.
* No confirmation dialog, the same as Controls.
* `docs/settings.md`: one line saying each Options tab has a reset that only touches that tab, and the window size
  stays.

## Tests

* Unit: the reset per tab leaves the other tabs' settings and the window size / fullscreen alone, and sets the
  rest to the defaults (pull the reset into a small function on `Settings` that tests can call without the menu).
* Scenario screenshot of the Display and Sound tabs with the button (`tests/scenarios/`), to check the layout fits
  under the rows.
