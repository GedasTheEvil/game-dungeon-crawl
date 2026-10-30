# Keyboard menus

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Opus: focus handling across all menu screens.

## Problems

* Main menu, Options, Save/Load and Credits are mouse-only: `MainMenu::KeyPressed` / `SpecialKeyPressed`
  (`src/ui/menu.cpp`) only forward keys to the Hall of Records. The death screen (`src/ui/death_screen.cpp`) and the
  inventory already have keyboard navigation.
* The journal's riddle "Answer" buttons need the mouse (`JournalScreen::SpecialKeyPressed` has no Enter).
* Inventory and Heka screens hard-code WASD, `E`, `U` (`src/ui/inventory.cpp`, `src/ui/skills_view.cpp`)
  instead of the player's bindings (`src/input/bindings.cpp`).
* Esc from a journal opened through the in-game menu goes straight to gameplay, not back to the menu
  (`input.cpp`, journal Esc).
* `docs/settings.md` says the journal moves with WASD; only the arrows work.

## Plan

* One focus model for button lists, tabs, option rows and slots: Up/Down (and the bound move keys) move the
  focus, Left/Right change an option value or switch tabs, Enter activates, Esc goes back one level. A visible
  focus highlight in the screen look ([ui.md](../ui.md)). Mouse hover moves the same focus.
* Journal: focus on the riddle Answer buttons with the arrows, Enter answers. Journal also takes the bound move keys.
* Inventory and Heka: the bound move / use keys, plus the arrows and Enter.
* Esc returns to the screen it came from (menu → journal → Esc → menu).
* Fix `docs/settings.md`.
* Gamepad support is out of scope; this focus model is the base it would need.
