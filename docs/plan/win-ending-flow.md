# Win ending flow

Status: implemented 2026-11-02, waiting for the user's play-test. Was: draft 2026-10-30, needs nothing first. From
the project audit of 2026-10-30.

Opus: end screen flow, a new journal page, input gating.

## Problems

* Esc at the win name entry drops the run from the Hall of Records without a warning
  (`src/ui/win_entry.cpp`, Esc branch).
* After that the player sees the old 3D win quad (`src/ui/end_screens.cpp`, `DrawWin`) with no prompt. Esc opens
  the in-game menu, which still offers "Return to Game" and "Save Game".
* F5 quick save works on a won game: it only checks that the player is alive (`src/input/input.cpp`, F5;
  `GameState::QuickSave`). A save is never meant to be of a won game (`LoadSave` clears the win).
* No epilogue. [journal-intro](solved/journal-intro.md) left "an epilogue on the win" for a later plan.

## Plan

* Esc at the name entry asks "Leave without entering the Hall of Records?" (the confirm box of
  [save-load-safety](solved/save-load-safety.md) if it exists by then, else a second Esc press with a hint line).
* After the name entry (or skipping it): the epilogue opens in the journal, like the prologue does at New Game
  (`journal.TakeIntro`, `ui.journal.OpenIntro`). A short text in `content/texts/journal.txt`: the archaeologist
  comes up into the dawn with the ankh. Fits the 12th hour: Ra is reborn as Khepri through the serpent
  "Life of the Gods" (ankh-netjeru), see [lore-additions](lore-additions.draft.opus.md).
* Closing the epilogue goes to the Hall of Records with the new row marked, then the main menu. Drop the 3D win quad.
* No quick save, no Save Game, no Return to Game once won.
* Play-test: a `tests/play/win-ending-flow.txt` that starts next to the ankh.

## Done

* `WinEntry` (`src/ui/win_entry.cpp`) runs the whole end: Typing, Epilogue, Done. The name panel now stands alone over
  the dimmed game with a title ("The Ankh of Life"); `EndScreens` and `media/textures/ui/win.png` are gone.
* Esc at the name entry opens the `ConfirmBox` ("Leave without entering the Hall of Records?"); its arrows are routed
  to it (special keys no longer reach the game while typing).
* The epilogue is a third diary page, `DiaryPage::Dawn` (`[diary.dawn]` in `content/texts/journal.txt`, the twelfth
  hour). `JournalScreen::OpenDiary` (was `OpenIntro`) opens the diary's last spread with the close hint ("J or Esc
  goes to the Hall of Records"). Closing the journal: the Hall of Records with the new row marked (none if skipped),
  in-game menu off. A run off the campaign (`--level`) skips the name entry and goes to the main menu.
* `GameState::Quicksave` and `Autosave` refuse a won game.
* Scenario `tests/scenarios/win_ending_flow.txt`, unit test in `journal_test.cpp`, play-test
  `tests/play/win-ending-flow.txt`.
