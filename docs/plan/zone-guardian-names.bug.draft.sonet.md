# Zone banner guardians named unlike their bosses

Status: draft 2026-11-13, size S, needs nothing first.

From the lore audit of 2026-11-13. The zone banner's guardian line (`ZoneLook::guardian`, `src/level/zone_look.h`,
shown by `src/ui/zone_banner.cpp` on a zone's first level) was written before two bosses got their names; it still
calls them by placeholders. Text only.

## Repro

1. `bin/game --level 10`: the banner of the web halls names the guardian "the spider queen of the webs".
2. Kill the boss of lvl13: the game, the journal, the readme and the level title call her "Mother of Webs".
3. `bin/game --level 38`: the banner says "the great worm of Sokar"; the boss of lvl41 is "Rerek" (level title "The
   Den of Rerek").

## Expected

The banner names each zone's guardian as the game names the boss, with an epithet like the other banners
("Khepri, the scarab who rolls the sun", "Apep, the serpent of chaos").

## Actual

* `src/level/zone_look.h:73`: `"the spider queen of the webs"`. The boss: `.name = "Mother of Webs"`
  (`src/level/monster_kinds.cpp:1292`), named by the user in [camel-spider-boss](solved/camel-spider-boss.md)
  ("Name: 'the Mother of Webs' ... not a god's name"); readme line 47 "the Mother of Webs, who puts out the torches";
  lvl13 "The Lair of the Mother of Webs".
* `src/level/zone_look.h:127`: `"the great worm of Sokar"`. The boss: `.name = "Rerek"` (`monster_kinds.cpp:1253`),
  named by the user in [worm-boss](solved/worm-boss.md); readme line 47 "Rerek, the sand serpent"; journal note
  "Rerek, the serpent of the underworld" (`content/texts/monsters.txt:119`).
* Also `readme.md:47`: the paragraph is headed "**A boss for every zone.**", but the necropolis has none (readme
  line 105 "all but the necropolis ending with a boss"; `ZoneLook::guardian` nullptr there).

## Fix

* zone_look.h:73: e.g. `"the Mother of Webs, who puts out the lights"`.
* zone_look.h:127: e.g. `"Rerek, the serpent of the sands"` (keeps "Sokar" out of it: the zone title already says
  "the Gate of Sokar"). Check the banner width (`zone_banner.cpp` fits its box to the longest line).
* readme.md:47: "**A boss for almost every zone.**" or "**A boss at the end of a zone.**".
* `grep -rn "spider queen\|great worm" src content readme.md docs/*.md`: none left.

## Fixed when

* A test that names the plan fails before the fix and passes after it: e.g. a unit test that each boss zone's
  `zoneLook(id).guardian` contains the `name` of its `ZONE_BOSS` monster kind, case-insensitive (the other nine
  pass today: "the scorpion queen, ..." holds "Scorpion queen"; this catches both, and any later rename).

## Play-test

* `bin/game --level 10`: the zone banner names the Mother of Webs.
* `bin/game --level 38`: the zone banner names Rerek.
