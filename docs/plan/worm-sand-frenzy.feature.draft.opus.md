# Sand worms are stronger on sand

Status: draft 2026-11-06, size M, needs nothing first.

## Idea

The sand worm in its own ground: on sand it moves at double speed, attacks 25% faster and hits 50% harder. On stone
it stays the slow worm of the early levels; in the desert's sand it becomes a real threat. The user's call: this
makes the worm stronger later in the game, on purpose.

## Plan

* "On sand": the worm's own cell is `Structure::Sand` or quicksand (`inSand`). Read per tick, so a worm crossing
  from sand to stone drops back at once.
* Movement: speed x2 on sand, the same pattern as the swimmer's `waterSpeed` (`MonsterKind`, monster_kinds.h): add a
  `sandSpeed` field (default 1), worm 2. Implementer's choice: the burrow's time under the sand
  (`SandBurrowRules::underMs`) also halves, or stays (the dive is already the sand move).
* Attack: `attackMs` x0.8 on sand (1000 -> 800 ms, 25% more blows), damage x1.5 (14 -> 21). Both as
  `MonsterKind` fields next to `sandSpeed` (e.g. `SandRules{speed, attackRate, damage}`), constants in
  gameplay_config.h. Its slowness curse is unchanged.
* Rerek (boss rule, AGENTS.md): his kin is the worm, so he gets it too, stronger where it makes sense: speed x2.25,
  attack x0.75, damage x1.5 (his whole hall is sand, so this is his fight, not a bonus; keep the damage at x1.5 or
  the swallow becomes a death sentence). Tune by play-test.
* The level checker: a worm spawned on sand counts more, like `SWIMMER_IN_WATER_THREAT_FACTOR` (level_check.cpp
  ~1319): add `WORM_IN_SAND_THREAT_FACTOR` (start ~1.6: about +88% damage per second and no escape). Run
  `bin/levelcheck content/levels/lvl*`; a level it now flags gets fixed (AGENTS.md, Campaign levels).
* Texts: the journal note `monster.worm` ("Slow as sand...") is now wrong: rewrite it (quick in the sand, slow on
  stone). The `description` in monster_kinds.cpp for both kinds, and the glossary "sand burrower" row.
* A tell (implementer's choice): a sand trail or a faster walk clip rate when it is quick, so the player sees why.
* Tests: a unit test (worm on sand vs on stone: speed, attack interval, damage; Rerek's factors).

## Balance notes

* The player walks at 3/4 in sand with no sprint, so a worm at x2 is ~2.7x the player's speed there: no running
  away in sand. That is the point, but sand halls with several worms may need fewer of them.
* Damage per second on sand: 14/s -> 26/s (x1.875). With 200 HP it is now roughly a mid-game monster in sand.

## Play-test

* 
