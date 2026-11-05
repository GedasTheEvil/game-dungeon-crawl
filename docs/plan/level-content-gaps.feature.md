# Level content gaps

Status: implemented 2026-11-05, waiting for the user's play-test. Was: draft 2026-10-30, size M, needs nothing first. From the project audit of 2026-10-30.

Opus: level edits. Every level must still pass `bin/levelcheck` with no warnings. Edit the level files by hand; never
regenerate (AGENTS.md, Campaign levels).

## Problems

* Inscription id 5, "Shezmu, lord of the press" (`content/inscriptions/campaign.txt`), is never placed. It is the
  hint for the Bat of Shezmu fight (it heals by biting). lvl9, the Shezmu level, has "The wheel of the day" instead.
  Every other id 1-137 is placed.
* Sunlight mirrors: only 4 mirrors and 4 sun discs, one each on lvl33, 34, 36, 37
  ([sunlight-mirrors](solved/sunlight-mirrors.md)). All 31 light shafts beam down; left, up and right never appear.
  The shafts on lvl35 and 38-41 are only daylight.
* Egg clusters only on lvl22 (2).

## Plan

* Place id 5 on lvl9 before the boss room (or swap it with "The wheel of the day" if that one fits elsewhere).
* Two to four more mirror puzzles in the desert zone and later levels, some with sideways shafts, a few with two
  mirrors. Keep the early ones simple.
* Egg clusters in one or two more spider / camel-spider levels, if the zone fits.

## Implemented

* **Id 5** ("Shezmu, lord of the press") on lvl9 at (25,13), on the walk to the teleporter into the Bat of Shezmu's
  roost. "The wheel of the day" stays where it was; the level has two inscriptions now.
* **Mirror puzzles** (three, edited by hand with the new `tools/level/lvledit.py`; [levels.md](../levels.md#single-cells-by-hand)):
  * lvl35: the blue key on the west ledge (now a small potion) became a blue sun disc at (6,25). A light shaft in
    the east wall (38,25) shines west along the hall's floor; the mirror at (14,25) sends it into the floor; one turn
    sends it on to the disc.
  * lvl38: the yellow lever became a yellow sun disc (33,24). A shaft in the west wall (2,24) shines east along the
    corridor (its potion moved a cell east). Two mirrors: the one by the shaft (12,24) points up (into the rock): turned
    edge-on the beam goes by; the green gate (26,24) stops it until opened; the one by the disc (31,24) sends it into
    the floor until turned east.
  * lvl40: the green lever became a shaft in the east wall (38,9), shining west. Two mirrors: (30,9) points into
    the floor, one turn sends the beam on; (17,9), west of the quicksand, turned up lights the green disc high on the
    wall over it (17,10).
  * Not lvl39 or lvl41: no spot where a disc fits a sideways beam without its own gate in the way. The checker counts
    one turn per puzzle (one Turn move sets every mirror the player reaches).
* **Egg clusters**: the zone fits scorpions, not spiders (the clusters are the scorpion queen's brood). Away from her
  they never hatch: a nest to cut (300 XP each). lvl20: three in the dead end east of the ladder at (29,9), by "The
  nest of eggs" ("Cut them while they are soft"); lvl21: two on the exit row's west side (3,3), (6,3).
  `zones_test` allowed the boss's nest on her own level only; now on every level of her zone.
* Tests: `tests/unit/level_mirror_puzzles_test.cpp` (each disc unlit as the level starts, lit after the turns), the
  scenario `tests/scenarios/level_content_gaps.txt` (lvl40 in the game, screenshots); decor goldens of lvl9, 35, 40.

## Play-test

`bin/game --play tests/play/level-content-gaps.txt` (lvl40, by the first mirror):

* The beam from the east wall ends at the mirror beside you. Turn it once (E): it runs on west over the quicksand.
* Go round to the second mirror (col 17, west of the quicksand), turn it twice: the beam climbs to the green disc high
  on the wall, the green gates open.
* `bin/game --level 35` (blue disc, one mirror), `bin/game --level 38` (two mirrors, the green gate in the beam's way),
  `bin/game --level 9` (Shezmu's text by the last teleporter), `bin/game --level 20` (the egg nest east of the
  ladder on the bottom floor): the full levels, if you have time.
