# Ending choice: immortality or renounce the gods and destroy the ankh

Status: solved 2026-11-23 (play-tested by the user). Was: implemented 2026-11-17, waiting for the user's play-test. Was: draft 2026-11-17, size M, needs nothing first.

## Idea

The user, 2026-11-17: instead of becoming immortal, the player can choose to renounce the Egyptian gods as demons and
destroy the ankh. The choice is recorded for the Hall of Records.

## Now

Using the ankh tile on lvl54 sets `won` (`Dungeon::useGate`, `src/sim/dungeon_interact.cpp`). `WinEntry`
(`src/ui/win_entry.cpp`) then runs name entry, the epilogue (`DiaryPage::Dawn`, `[diary.dawn]` in
`content/texts/journal.txt`: "Life without end"), the Hall of Records ([win-ending-flow](win-ending-flow.md)).
The scales verdict pages follow Dawn ([weighing-of-the-heart](weighing-of-the-heart.feature.md)).

## Plan

* **The choice.** Using the ankh asks first, before `won`: two options in the shared UI look ([ui.md](../../ui.md)),
  e.g. "Take the ankh: life without end" / "Renounce the gods as demons and break the ankh". Esc: back to the game,
  nothing chosen. Once picked, the choice is final (the user, 2026-11-17): no second ask, no undo. The `ConfirmBox` (`src/ui/confirm_box.h`) may fit with two labelled buttons, else a small panel.
* **Store it.** A `RunEnding` enum (`Immortal`, `Renounced`) on the run stats, set by the choice. Both still win the
  campaign (same score, same flow).
* **Destroy.** On Renounced: a short break of the ankh (a crack sound and the ankh model gone, or a flash), then the
  win flow. Keep it simple; no new model needed (hide the ankh mesh, a shatter of particles if one exists).
* **Epilogue.** A second dawn page for Renounced (`[diary.dawn-renounced]`): the archaeologist names them demons, breaks
  the ankh, comes up mortal into an ordinary sunrise; the twelfth-hour rebirth refused. The Immortal page stays
  `[diary.dawn]`. Check the scales verdict pages and D. J.'s answer
  ([expedition-ending](expedition-ending.feature.md)) still read right after the new page; adjust a line if one assumes
  the ankh was kept.
* **Hall of Records.** `RunRecord` gets `ending` (written to `saves/leaderboard.txt`; old records without it read as
  Immortal). Shown in the run details (an "Ending" row: "Immortal" / "Renounced the gods"); a small mark in the board
  row (e.g. a broken-ankh glyph or suffix) if it fits the columns.
* Glossary rows: the ending choice, Renounced.
* Tests: unit test for the record round-trip (with and without `ending`); a scenario that uses the ankh, picks
  Renounced and checks the record and the epilogue page.

Open: the exact wording of the two options and the page (the implementer's choice, in the journal's voice).

## Done

* `Screen::Ending` (`EndingScreen`), a panel over the dimmed, paused game: "Take the ankh: life without end" (focused
  first) / "Renounce the gods, break the ankh". Esc: back, nothing chosen. A pick calls `Dungeon::Win(RunEnding)`.
* Renounced: the ankh's cell emptied, the jar smash sound, a status line; the win flow follows 1.5 s later.
* `[diary.dawn-renounced]` replaces Dawn; the scales and D. J. pages still follow. D. J.'s back-cover answer said "I
  carried it out": now "I walked it to its end", true for both endings.
* `RunRecord::ending` (`ending=immortal|renounced`; missing: Immortal). An "Ending" column on the board, an "Ending"
  heading in the details (third column, above Explored: the Run column was full).
* The make-paths finale presses Enter at the choice. Scenario fields `ending_screen`, `ending`.

## Play-test

* `bin/game --play tests/play/ending-choice.txt` (starts next to the ankh, as win-ending-flow's did).
* Use the ankh: the choice appears; Esc returns to the game.
* Pick "Renounce": the ankh breaks, the renounced epilogue shows, the Hall of Records row and details say Renounced.
* Again with "Take the ankh": the old epilogue, the row says Immortal.
