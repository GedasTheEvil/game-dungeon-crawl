# Win ending flow

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

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
  [save-load-safety](save-load-safety.draft.opus.md) if it exists by then, else a second Esc press with a hint line).
* After the name entry (or skipping it): the epilogue opens in the journal, like the prologue does at New Game
  (`journal.TakeIntro`, `ui.journal.OpenIntro`). A short text in `content/texts/journal.txt`: the archaeologist
  comes up into the dawn with the ankh. Fits the 12th hour: Ra is reborn as Khepri through the serpent
  "Life of the Gods" (ankh-netjeru), see [lore-additions](lore-additions.draft.opus.md).
* Closing the epilogue goes to the Hall of Records with the new row marked, then the main menu. Drop the 3D win quad.
* No quick save, no Save Game, no Return to Game once won.
* Play-test: a `tests/play/win-ending-flow.txt` that starts next to the ankh.
