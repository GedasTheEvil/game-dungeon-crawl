# Endgame stats: skills learned with icons, favorite weapon, scrollable

Status: solved 2026-11-23 (play-tested by the user). Was: implemented 2026-11-23, waiting for the user's play-test. Was: draft 2026-11-20, size M, needs nothing first.

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
  clears them. Icons from the skill UI (`tools/where.py skill icon`), drawn in the shared UI look ([ui.md](../../ui.md)).
* Scrolling: wheel and arrow / PgUp / PgDn keys, a scrollbar like other scrollable screens (reuse one if it exists),
  content clipped to the panel.
* Hall of Records: store the favorite weapon in the entry if cheap; otherwise only the ending screen shows it.
* Scenario test in `tests/scenarios/`: a run with skills and weapon hits, screenshot of the stats top and scrolled.
* Glossary row for new terms ("favorite weapon").

## Done

* Favourite weapon = the most damage dealt (HP its hits took), not the most hits: a strong weapon used less can win,
  which says more about the run. Tie: the first item kind. Per weapon kind in `RunStats` (`WEAPONS` save line, old
  saves none); melee counts the weapon in hand, a shot the weapon that shot it (`Missile::weapon`); cheats count for
  none.
* The "endgame stats" are the run's details in the Hall of Records (the win flow opens it). The record stores
  `favorite_weapon` and `favorite_weapon_dealt`; old records show None.
* Skills: from the record's names (rank II kept), each with its Heka icon. Scrolling: the four columns together, by a
  row, wheel, Up / Down, PgUp / PgDn and the shared scroll bar; the "+ n more" cut is gone.
* Scenario `tests/scenarios/solved/endgame_stats_skills_weapon.txt`, unit tests in `tests/unit/leaderboard_test.cpp`.

## Play-test

* `bin/game --play tests/play/endgame-stats-skills-weapon.txt`: kill the rat with the dagger, use the ankh, choose,
  type a name, open the run in the Hall of Records.
* The details list the 11 skills with icons and "Favourite weapon: Flint Knife" with its damage.
* Wheel, Up / Down, PgUp / PgDn and dragging the bar scroll the columns; nothing drawn outside the panel.
