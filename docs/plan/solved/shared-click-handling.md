# Shared click handling

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30. Left from
[screens](screens.md) ("Not done": the press / hover / release handling stays twice).

Opus: a shared UI helper across six screens; screenshots must stay pixel-identical.

## Problem

Each screen keeps its own hovered / pressed state and its own press-on-down, fire-on-release-over-the-same-target
logic, with GLUT's `GLUT_DOWN` / `GLUT_UP`:

* `MainMenu` (`src/ui/menu.cpp`, `MouseFunction`): int targets, `hovered` / `pressed`, plus `dragging` for sliders and
  the wheel for the records list.
* `Inventory` (`src/ui/inventory.cpp`, `MouseFunction`): `Target` enum + `pressedSlot`; slots select on press and
  sink while held, right click uses at once, buttons fire on release, wheel scrolls.
* `SkillsScreen` (`src/ui/skills_view.cpp`): `Target` enum + `pressedCard`; cards select on press, Learn fires on release.
* `JournalScreen` (`src/ui/journal_view.cpp`): a `Hit` struct for `hovered` / `pressed`; hover is cleared when the page
  or screen changes.
* `DeathScreen` (`src/ui/death_screen.cpp`): int `pressed`, `clicked = at == pressed` on release.
* Screen tabs (`src/ui/screen_tabs.cpp`): `tabs.hovered` / `tabs.pressed`; hover cleared after a switch.

Six copies of the same rules; each new screen or button repeats them, and the bugs differ per copy. The
[keyboard-menus](../keyboard-menus.md) focus model would need a seventh.

## Plan

* One small helper in `src/ui/` (e.g. `ui::Pointer` / `ClickState<Target>`): holds `hovered`, `pressed`, the pressed
  mouse button; `Move(target)`, `Down(target, button)` returns whether the target selects on press,
  `Up(target)` returns the target to fire (pressed == released) or none; `Clear()` for page / screen switches; a
  `held(target)` query for drawing (hovered && pressed).
* Generic over the target type, so each screen keeps its own target enum / struct (int, `Target`, `Hit`).
* Keep the per-screen cases as options, not copies: select-on-press (inventory slots, skill cards), right click
  fires at once (inventory), drag (menu sliders), wheel (passed through to the screen).
* Move the six screens onto it one at a time; screenshot scenarios pixel-identical after each, all scenarios pass.
* Leave room for the keyboard focus of [keyboard-menus](../keyboard-menus.md): the focused target is the same
  slot as `hovered` (mouse moves it, keys move it), so that draft adds keys to the helper instead of a new copy.
* Unit tests for the helper: release over another target fires nothing, `Clear()` drops a held press, right click.
* Update [ui.md](../../ui.md) (how a screen handles clicks).

Code only, no gameplay or visual change: solved in the same commit once the checks pass (docs/plans.md, Solved).
