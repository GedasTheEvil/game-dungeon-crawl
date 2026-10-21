# Journal: hide the ribbons of empty sections

Solved 2026-10-21 (play-tested).

A journal section with no page yet has no ribbon: a new game's journal shows only the Diary's ribbon, the Creatures
ribbon comes with the first monster seen, the Riddles and Inscriptions ribbons with the first riddle or inscription.

- The ribbons don't move: each keeps its slot (`ribbonRect`), a hidden one is only not drawn (`JournalScreen::Shown`).
- A hidden ribbon can't be clicked or hovered; Up / Down skip hidden sections.
- The book never lies open on an empty section: it falls back to the first section with pages (old saves, the
  default Creatures section before any monster).

Test: `tests/scenarios/solved/journal_empty_ribbons.txt` (screenshots). Play-test: New Game, the journal opens with only
the Diary ribbon; see a monster, reopen the journal (J).
