# Anubis stuck under a step: backs off so its bolt clears

Status: draft 2026-11-08, size M, needs nothing first.

## Repro

1. A level with an Anubis guard and a step (a shaped cell, [level-subcells](solved/level-subcells.md)) the player can
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

* Existing pieces to reuse: the kiting back-off ([monster-kiting](solved/monster-kiting.md): `Monster::BackOff`,
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
  they have it. [subcell-campaign](solved/subcell-campaign.feature.md) uses steps as cover against shooters on purpose
  (crouch behind a step): keep that cover, the shooter just has to move for a line, not shoot through.
* Checker: `monsterThreat` / the reckless threat ([checker-reckless-threat](solved/checker-reckless-threat.tooling.md)) may
  count such steps as safe spots; update it if the rule changes what the checker models.

## Fixed when

* A test that names the plan (unit or scenario): the player on a step over an Anubis guard that cannot reach them; within
  a few seconds Anubis moves back and a bolt reaches the player (not splatting on the step). Fails before the fix.
* The same for the Anubis boss.
* In open ground Anubis still chases and swings, no back-off.

## Play-test

* A play script in `tests/play/anubis-stuck-under-step.txt` puts the player on a step with an Anubis guard below.
  Stand there and poke it with a spear: it steps back and shoots a bolt that hits you.
* Same with the Anubis boss: it gets out from under the step faster.
