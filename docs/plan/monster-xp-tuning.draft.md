# Monster XP tuning

Status: idea, not started.

## What

* Some monsters give too much XP for how hard they are to kill. Nerf them, so XP follows the real danger.
* Found in play: the mummy gave too much at 4500 and was cut to 2500 (lvl11, beaten with a sword,
  [solved/mummy-minion-monster.md](solved/mummy-minion-monster.md)). Others are likely off too.

## Current values

From `MONSTER_DEFS` (`src/state/assets.cpp`). Speed, HP, damage, ms between attacks, XP.

| Monster | Speed | HP | Dmg | Attack ms | XP | Note |
|---|---|---|---|---|---|---|
| Scarab | 4 | 10 | 2 | 600 | 300 | |
| Rat | 9 | 12 | 2 | 400 | 300 | |
| Bat | 5 | 8 | 3 | 800 | 400 | |
| Plant | 0 | 30 | 5 | 800 | 1000 | does not move |
| Worm | 1 | 30 | 9 | 1000 | 1200 | |
| Mimic | 0 | 40 | 10 | 800 | 1500 | does not move, leaves a chest |
| Giant bat | 4 | 40 | 10 | 800 | 1800 | |
| Giant rat | 4 | 60 | 8 | 700 | 2000 | leaps |
| Giant scarab | 2 | 90 | 12 | 1000 | 2500 | leaps |
| Mummy | 2.5 | 150 | 20 | 1600 | 2500 | slow to wake, telegraphed |
| Boss scarab | 3 | 320 | 40 | 900 | 6000 | boss |
| Anubis | 3 | 350 | 30 | 1200 | 10000 | |
| Vampire bat | 4 | 400 | 80 | 800 | 12000 | boss, heals |

Boss minions get their XP from `Dungeon::MinionXP`.

## Ideas

* Score each monster from HP, damage per second and mobility (the checker's `monsterThreat` is a start), and set its XP
  from that score.
* Suspects: the plant and the mimic (they never move, so the player picks the fight), Anubis (10000 vs 350 HP).
* Check the effect on the level curve: the player level when reaching lvl5 / lvl10 bosses is used in the boss tuning
  (`solved/boss-rooms.md`). Rerun a full playthrough or a scenario that sums the XP per level.

## Open questions

* Which monsters to nerf, by how much?
* Should the bosses keep their XP (they are the payoff)?
