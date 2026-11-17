# Ending choice: immortality or renounce the gods and destroy the ankh

Status: draft 2026-11-17, size M, needs nothing first.

## Idea

The user, 2026-11-17: instead of becoming immortal, the player can choose to renounce the Egyptian gods as demons and
destroy the ankh. The choice is recorded for the Hall of Records.

## Now

Using the ankh tile on lvl54 sets `won` (`Dungeon::useGate`, `src/sim/dungeon_interact.cpp`). `WinEntry`
(`src/ui/win_entry.cpp`) then runs name entry, the epilogue (`DiaryPage::Dawn`, `[diary.dawn]` in
`content/texts/journal.txt`: "Life without end"), the Hall of Records ([win-ending-flow](solved/win-ending-flow.md)).
The scales verdict pages follow Dawn ([weighing-of-the-heart](solved/weighing-of-the-heart.feature.md)).

## Plan

* **The choice.** Using the ankh asks first, before `won`: two options in the shared UI look ([ui.md](../ui.md)),
  e.g. "Take the ankh: life without end" / "Renounce the gods as demons and break the ankh". Esc: back to the game,
  nothing chosen. The `ConfirmBox` (`src/ui/confirm_box.h`) may fit with two labelled buttons, else a small panel.
* **Store it.** A `RunEnding` enum (`Immortal`, `Renounced`) on the run stats, set by the choice. Both still win the
  campaign (same score, same flow).
* **Destroy.** On Renounced: a short break of the ankh (a crack sound and the ankh model gone, or a flash), then the
  win flow. Keep it simple; no new model needed (hide the ankh mesh, a shatter of particles if one exists).
* **Epilogue.** A second dawn page for Renounced (`[diary.dawn-renounced]`): the archaeologist names them demons, breaks
  the ankh, comes up mortal into an ordinary sunrise; the twelfth-hour rebirth refused. The Immortal page stays
  `[diary.dawn]`. Check the scales verdict pages and D. J.'s answer
  ([expedition-ending](solved/expedition-ending.feature.md)) still read right after the new page; adjust a line if one assumes
  the ankh was kept.
* **Hall of Records.** `RunRecord` gets `ending` (written to `saves/leaderboard.txt`; old records without it read as
  Immortal). Shown in the run details (an "Ending" row: "Immortal" / "Renounced the gods"); a small mark in the board
  row (e.g. a broken-ankh glyph or suffix) if it fits the columns.
* Glossary rows: the ending choice, Renounced.
* Tests: unit test for the record round-trip (with and without `ending`); a scenario that uses the ankh, picks
  Renounced and checks the record and the epilogue page.

Open: the exact wording of the two options and the page (the implementer's choice, in the journal's voice); whether
the choice is final once picked (suggested: yes, no second ask).

## Play-test

* `bin/game --play tests/play/ending-choice.txt` (starts next to the ankh, as win-ending-flow's did).
* Use the ankh: the choice appears; Esc returns to the game.
* Pick "Renounce": the ankh breaks, the renounced epilogue shows, the Hall of Records row and details say Renounced.
* Again with "Take the ankh": the old epilogue, the row says Immortal.
