# Quick save and quick load

Status: draft 2026-10-29, from the user's idea. Needs nothing first.

## Idea

One key saves the game at once, another loads it back, no menu: F5 saves, F9 loads.

## Rules

* **The slot:** a special one, `saves/quicksave.sav` (same format, `GameState::Save` / `LoadSave`), like the autosave
  not one of the six slots (`src/state/save_slots.h`). A scenario's goes to its output directory, as the autosave's
  does.
* **F5, quick save:** only in gameplay (`Screen` is the game; not in a menu, riddle, map, journal, death screen),
  only while the player is alive. Overwrites the old quick save. Toast "Quick saved" (same toast as "Autosaved").
* **F9, quick load:** in gameplay and on the death screen. With no quick save: toast "No quick save", nothing else.
  Loading resets the death state as any load does.
* **Keys:** fixed, like F1 / F3 (`isReservedKey` already reserves F1-F11, so no binding can take them). Add them to
  the Controls tab's "Fixed:" line (`src/ui/menu.cpp`).
* **Load Game list:** shows the quick save right after the autosave, read-only, "Quick save, level N" with its time
  (`SaveSlots::DescribeAutosave`-like helper; generalise it rather than copy).
* Plan retry-latest-save (if done first): the quick save is a candidate there.

## Tests

* Scenario: F5, move, F9 → back at the saved spot; F9 with no file → toast, no load; F5 while dead → no file.
* Play script `tests/play/quick-save.txt` for the user's play-test.
