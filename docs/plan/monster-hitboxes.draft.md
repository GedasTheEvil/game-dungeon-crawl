# Monster hitboxes

Status: bug found in play (lvl5, the boss scarab, 2026-09-30). Not started.

## The bug

With the sword, standing right next to the boss scarab and taking its bites, most swings did no damage. The
boss lost at most ~30% of its HP in a whole fight.

Cause: every range check treats a monster as a point at its centre (`Monster::CentreX()`), whatever its size.
The monster's own stop distance grows with its size, but the player's reach does not:

| Check | Code | Rule today |
|---|---|---|
| Melee hit | `Monster::Nearby` (`monster.cpp`), called by `Dungeon::AttackNearest` | centre within `-MELEE_REACH_BEHIND` .. `range / 10` tiles ahead of the player |
| Monster stops and bites | `Monster::attackDirection` (`monster_ai.cpp`) | stops when the centre is `0.05 + 0.02 x scale` tiles from the player |
| Arrow hit | `Dungeon::updateArrows` (`dungeon_arrows.cpp`) | `abs(CentreX - arrow x) <= ARROW_HIT_HALF_WIDTH` (0.3), height from `BottomY` / `TopY` |
| Bow aim | `Dungeon::aimTarget` | aims at the centre, range to the centre |
| Walls | `MONSTER_WALL_MARGIN` (0.5) | the same half width for every monster |

Stop distance (centre to player) against melee reach (club 0.4 tiles, sword 0.5; spear 0.8):

| Monster | Scale | Stops at | Club / sword can hit |
|---|---|---|---|
| Scarab | 7 | 0.19 | yes |
| Rat | 13 | 0.31 | yes |
| Worm | 18 | 0.41 | sword only, just |
| Giant scarab | 24 | 0.53 | spear only |
| Boss scarab | 34 | 0.73 | spear only |
| Giant rat | 42 | 0.89 | no weapon |

A monster that stops at 0.73 tiles is only hit when it lurches a little nearer between steps, which explains
the rare hits. Walk-jumpers land on the player's cell centre (the comment on `MONSTER_DEFS` says their reach must be
over half a tile), which briefly puts them in range.

## Goal

Each monster gets a box in map units (tiles) that matches its drawn model and scales with it: half width, bottom and
top. Every check measures to the box edge, not the centre. Standing where a monster stops and bites, every melee
weapon hits it.

## Plan

1. **Measure the box.** In `CharacterModel::Load`, next to `referenceTop`, record the frame 0 X extent (and Z, the
   depth, for reference) of the reference clip. `MonsterType` gets `halfWidth`, `bottom` and `top` in tiles:
   extent x `scale` x `Ink::figureScale()` / `TILE_SIZE`. Facing turns the model (`rotA + 90 x facing`), so take the
   extent along the axis that faces the corridor, or the larger of X and Z, whichever matches what the player sees.
   Check the drawn centre against `CentreX()` first (`Monster::Draw` translates by `40 x - 20` plus
   `MONSTER_OFFSET_X`). If they differ, fix that before anything else.
2. **Box on the monster.** `Monster::Left()` / `Right()` (`CentreX() -/+ halfWidth`); `BottomY` / `TopY` stay, and use
   the same box.
3. **Melee:** `Nearby` measures from the player to the near edge. The monster is hit if its near edge is within
   `range / 10` ahead and its far edge is not behind the player by more than `MELEE_REACH_BEHIND`.
   `AttackNearest` picks the nearest by edge distance.
4. **Monster reach:** `attackDirection` stops the monster when its near edge is `MONSTER_BITE_REACH` from the player
   (one constant, ~0.1 tiles), instead of `0.05 + 0.02 x scale`. The bite needs no change: it only needs the same
   row. This also puts the stop distance where the eye expects it (jaws at the player), not at the centre.
5. **Arrows:** hit when the arrow x is inside `[Left, Right]` (plus a small tolerance) and between bottom and top.
   `aimTarget` aims at the near edge's height centre and measures range to the near edge.
6. **Walls:** `seekProbeX` uses `halfWidth` instead of `MONSTER_WALL_MARGIN`, so big monsters stop before a wall
   instead of sinking into it. Check the leap (`leapLanding`, `Monster::Jump`) still lands the giant rat and scarabs
   on a cell they fit on.
7. **Flyers:** a bat's box follows its flight lift. The swoop bite already passes through the player, so check it
   still bites, and that arrows and swings hit a roosting or swooping bat where it is drawn.
8. **Debug view:** scenario command `hitboxes on|off` (and a debug key in a dev build, if wanted) draws each
   monster's box and the player's weapon reach as outlines, to line them up with the models in screenshots.

## Tests

* A scenario per monster size (scarab, worm, giant scarab, boss scarab, giant rat): the monster walks up and stops;
  the player, not moving, swings each melee weapon. Expect the monster's HP to drop on every swing (a new
  `expect` field for the nearest monster's HP, or `bars` plus HP of the boss via `expect boss`).
* The same with the bow: an arrow at a big monster standing 2 tiles away hits it.
* `hitboxes on` screenshots of every monster, both facings, for review.
* Balance check afterwards: with real hits, the boss scarab (320 HP) and the giants may now die much faster. Replay
  lvl5 and a giant rat level, tune HP in `MONSTER_DEFS` and `BOSS_DEFS` if needed.

## Open questions

* Should the player get a box too (monster bites and traps check the player's box, not the cell), or is "same row
  and in reach" enough?
* The spear's thrust (0.8 tiles) was presumably tuned against the old point hitbox. After this change the reach of all
  three weapons may need a second look (club 0.4, sword 0.5).
