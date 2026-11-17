# Lesser Anubis guard before the necropolis, a stronger one in it

Status: implemented 2026-11-17, waiting for the user's play-test. Was: draft 2026-11-16, size M, needs nothing first.

## Idea

From the user: a lesser Anubis guard, the Anubis living statue at half size (0.5x), for the levels before the
necropolis. It has no ranged attack and 0.75 of today's guard's HP. The Anubis guard of the necropolis gets 3x the HP
and 2x the attack of today's guard.

Today (`src/level/monster_kinds.cpp`, `MonsterAnubis`, glyph `n`): 1300 HP, damage 140, a fire bolt, scale 19.
Placed in zone 4, the priests' quarter (lvl14-18, "the first Anubis guards"), and in zones 11-12 (necropolis lvl46-50,
Hall of Judgment lvl51-54) (`src/level/campaign.h`, the seed table).

## Plan

* **New kind** `MonsterLesserAnubis` (append to the monster enum in `src/level/level.h`, never reorder), a free glyph
  (pick one, `bin/levelcheck --legend`). Name like "Lesser Anubis statue" (or "Small Anubis statue"; implementer's
  choice, same family as the Anubis living statue, [egyptian-names](solved/egyptian-names.md)). Shares the guard's
  model, texture and clips ([shared-assets](solved/shared-assets.md): the rat and giant rat pattern).
* **Lesser:** scale 0.5x of the guard's, 975 HP (0.75x), no `ranged` (no bolt, so no `rangedCurses`), melee damage:
  keep 140 or lower (open point, below). Everything that is a length scales with it: `climbRise`, `climbGrip`,
  `walkStride` (or the feet slide), melee reach and height (`HIGH_BLOW` likely becomes a mid blow at half height:
  [crouch](solved/crouch.md), [monster-attack-reach](solved/monster-attack-reach.md)), its sub-cell size class
  ([subcell-monsters](solved/subcell-monsters.md): at half height it may fit crawl spaces). Threat, XP, trap share,
  ring drop: lower, tuned. Still bloodless, poison immune, reckless, a climber.
* **Necropolis guard:** `MonsterAnubis` gets 3900 HP (3x) and damage 280 (2x). Its bolt: 2x as well (open point). XP
  up to match. Threat in the checker (`monsterThreat`) up.
* **Placement:** zone 4 (lvl14-18) and every guard before lvl46 becomes the lesser one; the generator's zone 4 row in
  `src/level/campaign.h` too (and the `generated` depth of both kinds). Edit the level files by hand
  (`tools/level/lvledit.py`), never regenerate. Every campaign level passes `bin/levelcheck` with no warnings after
  the threat change; lvl46-54 likely need fixes (fewer guards, more cover).
* **The Anubis boss** (`MonsterAnubisBoss`, kin `MonsterAnubis`): today 2400 HP, damage 190, so the new guard would
  outclass the final boss. From the user: the boss gets strong as well, 3x HP and 2x damage (7200 HP, 380), its bolt
  2x too; it stays above its kin.
* Journal: a field note for the lesser statue (or the guard's note covers both), the manual's monster list.

### Open points (implementer's choice; warn the user)

* "2x attack": melee only, or the bolt too? Default: both.
* No one-shot risk (the user, play-testing 2026-11-16): the player reaches the necropolis with about 3500 HP, Might 31,
  Armor 78 and a mace of level 4. Take 3x / 2x as given; balance against that player.
* Lesser melee damage: the user gave HP only. Default: 0.75x as well (105), it is half the size.
* Late zones 5-10 have no guard today; the lesser one could appear there too. Default: no, only where guards are now.

## Play-test

* `bin/game --level 14`: the lesser statue, half height, walks and swings, never shoots.
* `bin/game --level 46`: the full guard takes about three times the hits and hurts twice as much.
* The Anubis boss (lvl54) still outclasses its guards.

## Done

* `MonsterLesserAnubis` (37, glyph `9`), "Lesser Anubis statue", label `lesser anubis`: the guard's model, texture and
  clips at scale 9.5 (0.5x), 975 HP, blow 105 (0.75x, the default), no bolt, no ranged curses, Vulnerability 10%. Its
  blow is a body blow (`LESSER_ANUBIS_BLOW`, a crouch does not duck it), `heightQuarters` 1 (fits a crawl space). Same
  speed as the guard. Threat 7, XP 6000, trap share 25, weight 2, ring drop 4%. Strides and climb clip numbers are in
  model units: unchanged. Generated at difficulty 7-8; the priests' quarter's zone row.
* Guard: 3900 HP, blow 280, bolt 140 (both doubled, the default), XP 25000, threat 20.
* Anubis boss: 7200 HP, blow 380, bolt 250, XP 60000, threat 30.
* lvl14-18: the 11 guards are lesser statues now (lvledit). Zones 5-10 untouched (the default). lvl46-54 unchanged.
* Field note `monster.lesser-anubis`, glossary row, `docs/levels.md`. The manual shows no Anubis: unchanged.
* Tests: `tests/unit/lesser_anubis_test.cpp`; the crouch test covers the lesser statue.
