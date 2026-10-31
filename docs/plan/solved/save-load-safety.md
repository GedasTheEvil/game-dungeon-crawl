# Save and load safety

Status: solved 2026-10-31 (play-tested by the user). From the project audit of 2026-10-30.

Opus: a confirm dialog in the shared UI look ([ui.md](../../ui.md)), several menu flows, save error handling.

## Problems

1. **New Game overwrites the autosave at once.** `GameState::NewGame` calls `Autosave(LevelStart)`
   (`src/state/game_state.cpp`, after `player->Reanimate()`). There is one autosave file
   (`src/state/save_slots.h`), and the New Game button has no confirmation (`src/ui/menu.cpp`, New Game). One misclick
   loses a long run that was not saved in a manual slot.
2. **A failed save still says "Saved to slot N".** `GameState::Save` only logs when the file can't be opened and
   returns void; the menu records the slot and shows the success toast anyway (`menu.cpp` `saveToSlot`).
3. **A failed load leaves a broken state.** `GameState::LoadSave` calls `player->Reanimate()` and
   `dungeon.ClearWin()` before it checks the file opens; the menu then switches to gameplay regardless.
4. **No confirmation for actions that throw away progress**: in-game "Main Menu" (only sets `inGame=false`), Load
   Game while playing, Exit, overwriting a slot (only a red "Overwrite" label on hover).
5. **Slots show only the level and the date.** No play time, no character level; slots cannot be deleted.

## Plan

* A small confirm box (question, Yes / No, Enter / Esc) in the screen look, reusable by every flow below.
* New Game while an autosave of a live run exists: ask "Start a new game? The current run's autosave is replaced."
  Optionally keep the old autosave as `autosave.bak` (one level of undo); implementer's choice.
* `Save` and `LoadSave` return success. Save: write to a temp file, rename over the target (no half-written save
  on a crash), toast "Could not save" on failure. Load: open and parse first, only then touch the game state; on
  failure stay in the menu with "Could not load slot N".
* Confirm Main Menu / Load / Exit while in a live game when there is progress since the last save (time or events
  since the last autosave or manual save); no box when nothing would be lost.
* Confirm overwriting a used slot. Slot rows show level, zone, character level and play time
  (`RunStats` has the time). A delete action per slot, with confirmation.
* Unit tests: failed save reports failure and leaves the old file; failed load leaves the state untouched.

## Done

* `ConfirmBox` (`src/ui/confirm_box.*`, docs/ui.md "Confirm box"); `MainMenu::AskOrDo`.
* New Game asks while an autosave exists. No `autosave.bak`: nothing could load it.
* The save file goes through `world/dungeon_save.*` (no GL, unit-tested): `writeFileSafely` writes a temp file and
  renames it; `loadSaveGame` reads into the game and, on a broken save, puts back the state it had (written and read
  as a save is: what a save leaves out, such as a burning player, starts over). New saves start with `SAVEGAME` and
  end with `END`, so a cut one is caught; older saves have neither and load as before.
* Main Menu and Load Game ask after 3 s or more of play since the last save or load (`GameState::UnsavedProgress`).
  Exit does not ask: it is only on the main menu, after the run was already left.
* Slots: level and zone, date, character level and play time, read from the file's head (cached by file time and
  size); `saves/gamelist.dat` is gone (it only held the level). A cross on a used slot, or Delete, deletes it after a
  question.
* Death screen: a Retry whose save cannot be loaded turns into "Could not load ..." and is disabled.
* Tests: `tests/unit/save_load_safety_test.cpp`, scenario `tests/scenarios/solved/save_load_safety.txt`.
