# Anubis stuck under a step: backs off so its bolt clears

Status: solved 2026-11-09 (play-tested by the user). Was: implemented 2026-11-08, waiting for the user's play-test. Was: draft 2026-11-08, size M, needs nothing first.

## Repro

1. A level with an Anubis guard and a step (a shaped cell, [level-subcells](level-subcells.md)) the player can
   stand on but Anubis cannot reach with its melee.
2. Stand on the step, Anubis comes right under it.
3. Hit it with a spear (reach) over and over.

## Expected

Anubis does not stand there taking hits. Its melee cannot reach, so it at least tries its bolt from a spot where the
bolt reaches the player: it backs off along the row (away from the step's face) until the line from its release point
to the player's chest clears the step, then shoots. Cornered (no room behind), it does what it does today.

## Actual

Its melee misses (the player is above its reach), so it falls back to the bolt (`RangedRules`, range 1.4 tiles, from
the was-sceptre, `src/level/monster_kinds.cpp`). Shot from under the step, the bolt's straight line hits the step's
corner and splats (`Dungeon::updateShots`, `src/sim/dungeon_arrows.cpp`). Anubis stays where it is
(`Courage::Reckless`, no `kite` rules), so the player hits it with a spear safely, as long as they like.

## Notes

* Existing pieces to reuse: the kiting back-off ([monster-kiting](monster-kiting.md): `Monster::BackOff`,
  `Dungeon::kiteRoom`, `kiteAway` in `src/sim/dungeon_walkers.cpp`, `dungeon_monsters.cpp` ~118-150). Anubis is not a
  kiter and must not become one in open ground (it chases and swings); this is a narrower rule: "melee cannot reach
  and the bolt's line is blocked by terrain: step back until it is clear, within the bolt's range".
* The line test: from the release point (`Monster` release in `monster_ai.cpp` ~250: `releaseAhead`, `releaseY`) to
  where `aimShot` aims (the chest), against the cells' shapes (`solidAt`). `clearRow` today only checks walls along the
  row, not the step's sub-cells; extend it or add a shape-aware sight line.
* If no spot within range clears the line, or no room behind (wall, drop, trap, `kiteRoom`), it may retreat out of
  the spear's reach instead of standing in it. Implementer's choice; say which in the plan.
* Boss: the Anubis boss (`kin` anubis) gets the same rule, stronger where it makes sense (backs off faster, or its
  bolt aims higher / lobs over the edge). AGENTS.md, Boss abilities.
* Other shooters with the same hole (cobra spit, baboon, Babi; `ranged` without `kite`): check them; same fix if
  they have it. [subcell-campaign](subcell-campaign.feature.md) uses steps as cover against shooters on purpose
  (crouch behind a step): keep that cover, the shooter just has to move for a line, not shoot through.
* Checker: `monsterThreat` / the reckless threat ([checker-reckless-threat](checker-reckless-threat.tooling.md)) may
  count such steps as safe spots; update it if the rule changes what the checker models.

## Done

* The rule ("line up", glossary), for every walker with a ranged attack and no kite rules: the Anubis, its boss, the
  mummy priest (cobras and Apep do not walk; the baboon and Babi are kiters). It applies when the shooter cannot bite
  the player: a step it cannot take on the way (`walksToBite`: `walkerReaches` missed a step in the player's own cell),
  or its swing from the bite spot misses them up there (`Monster::ReachesFrom`; also with the player in bite reach by
  the row, `attackDirection` 0, the draft's "right under it"). `Dungeon::lineUpMove` (`src/sim/dungeon_walkers.cpp`):
  * the line from its release point (`Monster::ShotRelease`) to the player's chest meets a wall or a solid sub-cell
    (`shotLineClear`, as `updateShots`, ending where `Player::Covers` the shot): it steps back, facing the player
    (`Monster::LineUp`), while a spot behind it, within its range and its free run (`kiteRoom`), clears the line;
  * lined up, it holds there while one more step on would lose the line (no shuffle back under the step);
  * no spot in range clears it, the player up on a step (implementer's choice): it backs off to its range's edge and
    waits there, out of a spear's reach. Behind cover on its own floor (crouched behind a step) or cornered, it does
    what it did: cover stays cover;
  * no shot while it still has to step back (the gate in `UpdateMonsters`).
* It runs before the climber's path (`followPath`): a shot from a spot in range beats a long climb round.
* Boss: `LINE_UP_BOSS_PACE` (1.5) times its walk step backing off; its bolt's range (2.0) finds lines its guard's (1.4)
  does not. On a three-quarter step its bigger swing still reaches a standing player; crouched (a high blow ducked) it
  lines up.
* Checker: no change. `monsterReaches` models where a monster walks, not its shots; the rule changes no walk.
* Tests: `tests/unit/anubis_step_test.cpp` on `tests/levels/anubis_step` and `anubis_step_boss` (three fail before the
  fix: the guard's and the boss's bolt hit, the retreat to its range's edge). Play scripts: `tests/play/`
  `anubis-stuck-under-step.txt` and `anubis-stuck-under-step-boss.txt`.

## Fixed when

* A test that names the plan (unit or scenario): the player on a step over an Anubis guard that cannot reach them; within
  a few seconds Anubis moves back and a bolt reaches the player (not splatting on the step). Fails before the fix.
* The same for the Anubis boss.
* In open ground Anubis still chases and swings, no back-off.

## Play-test

* `bin/game --play tests/play/anubis-stuck-under-step.txt`: the script takes you up the steps to the edge of a step
  three quarters high, a spear in hand (god mode). Poke the Anubis below: it steps back out of the spear's reach and
  its bolt hits you, not the step.
* Step back from the edge (towards the left end of the step): it backs off to the edge of its range and waits.
* `bin/game --play tests/play/anubis-stuck-under-step-boss.txt`: the same with the boss, the down key held (its high
  swing ducked): it backs off faster and its bolt hits you.
