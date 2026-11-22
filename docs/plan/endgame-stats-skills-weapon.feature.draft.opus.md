# Endgame stats: skills learned with icons, favorite weapon, scrollable

Status: draft 2026-11-20, size M, needs nothing first.

## Idea

The user, 2026-11-20: the endgame stats (the run's numbers shown at the end: `src/ui/ending_screen.cpp`, the counters
in `RunStats`, `src/level/run_stats.h`; the Hall of Records `src/ui/records_view.cpp`) also show:

* The skills learned this run, each with its icon (the Heka skill icons).
* The favorite weapon: the weapon that dealt the most damage (or the most hits; implementer picks, say which).
* The screen scrolls, as the list gets longer than the panel.

## Plan

* Count per weapon in `RunStats` (damage dealt by weapon kind, where `damageDealt` is counted); it rides in the save
  game like the other counters (old saves: zero, ignored). Favorite = the max; none if no weapon hit (fists).
* Skills learned: read from the player's learned skills at the end; record them in the run stats if the win flow
  clears them. Icons from the skill UI (`tools/where.py skill icon`), drawn in the shared UI look ([ui.md](../ui.md)).
* Scrolling: wheel and arrow / PgUp / PgDn keys, a scrollbar like other scrollable screens (reuse one if it exists),
  content clipped to the panel.
* Hall of Records: store the favorite weapon in the entry if cheap; otherwise only the ending screen shows it.
* Scenario test in `tests/scenarios/`: a run with skills and weapon hits, screenshot of the stats top and scrolled.
* Glossary row for new terms ("favorite weapon").

## Play-test

* `bin/game --play tests/play/endgame-stats-skills-weapon.txt` (a script that gives skills, weapons and wins).
* The stats list the learned skills with icons and the favorite weapon.
* Wheel and keys scroll the list; nothing is drawn outside the panel.
