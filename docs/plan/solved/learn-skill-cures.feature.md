# Learning Serket's favour cures poison, Blessing of Isis lifts curses

Status: implemented 2026-11-11, waiting for the user's play-test. Was: draft 2026-11-11, size S, needs nothing first.

## Idea

From the user. The moment the player learns a skill on the Heka screen, its god acts once:

* Serket's favour: a running poison is cured.
* Blessing of Isis: every running curse is lifted (greater ones too), as a prayer at a Shrine of Isis does.

One-off on learning only; no ward starts (the Shelter of Serket and the Protection of Isis stay the shrines'). Nothing
happens when the player is not poisoned / not cursed.

## Plan

* In `PlayerStats::Learn` (`src/level/player_stats.cpp`), after a successful `heka.Learn`: `SerketsFavour` →
  `vitals.poison.Cure()`; `BlessingOfIsis` → `EndCurses()`. Same calls as `PlayerStats::Pray` for `God::Serket` /
  `God::Isis`. Return or expose whether something was cured, so the UI can say it.
* `SkillsScreen::PressLearn` (`src/ui/skills_view.cpp`): append to the "Learnt: ..." toast when it cured something,
  e.g. ". The poison is gone" / ". The curses lift" (same wording as the shrine lines).
* The scenario `learn` command goes through `PlayerStats::Learn` too: check it gets the cure (it should, no extra code).
* Unit test (`tests/unit/`): poisoned player learns Serket's favour → not poisoned; cursed player (two curses, one
  greater) learns Blessing of Isis → no curses; learning another skill cures nothing. Name the plan in the test.
* Glossary: the "skill names" row (Serket's favour, Blessing of Isis) gets "learning it cures the poison / lifts the
  curses". Update the skills' field notes in `src/level/journal.cpp` (`skillNote`) if they list what the skill does.

## Play-test

* `bin/game --play tests/play/learn-skill-cures.txt` (setup: a heka point free, `poison`, `curse` two kinds).
* Open the Heka screen, learn Serket's favour: the poison card goes, the toast says the poison is gone.
* Learn Blessing of Isis: the curse cards go, the toast says the curses lift.
