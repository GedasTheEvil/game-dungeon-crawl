# Sand worm walks 1.5x faster

Status: solved 2026-11-15 (play-tested by the user). Was: implemented 2026-11-15, waiting for the user's play-test. Was: draft 2026-11-15, size S, needs nothing first.

## Idea

The sand worm moves far too slowly (user, 2026-11-15). It should walk 1.5x faster, on plain ground and in sand.

## Plan

* `MonsterWorm` in `src/level/monster_kinds.cpp`: `.speed = 1` becomes `.speed = 1.5f`. The walk step is
  `MONSTER_SEEK_STEP * type->speed * terrainSpeed()` (`Monster::WalkStep`, `src/sim/monster_ai.cpp`), and in sand
  `terrainSpeed()` multiplies it by the native sand factor (`TERRAIN_ROWS`, x2), so one change makes it 1.5x faster
  on both ground and sand. Do not touch `TERRAIN_ROWS`: other kinds share the sand row.
* The worm's walk clip follows its speed by itself (`walkStride`), so the crawl should not slide. Check it in the
  scenario screenshots. If it slides, adjust `WORM_STRIDE`, but Rerek uses it too, so his crawl must still look
  right.
* The time it spends under the sand (`BURROW_UNDER_MS`, `SandBurrowRules`) is not a walk speed. Leave it as is.
* Rerek (boss, `kin` MonsterWorm, `.speed = 3`) stays faster than the worm. No change needed. Check that he still
  walks faster (3 > 1.5).
* `.threat = 2.f`: raise it a little for the faster walk (e.g. 2.3), then run `bin/levelcheck content/levels/lvl*`.
  Any level that now warns (threat budget) is a level fix, per AGENTS.md.
* Test: a unit test that names the plan. A worm's `WalkStep` on plain floor is 1.5x the old one
  (`MONSTER_SEEK_STEP * 1.5`), and in sand it is the sand factor times that.

## Play-test

Run: `bin/game --no-sound --play tests/play/worm-speed.txt` (a floor worm at col 28, sand worms at cols 7 and 21).


* A level with worms on plain floor and in sand (e.g. a desert level). Out of sand the worm keeps up better and is
  still beatable. In sand it closes in noticeably faster. Its crawl clip matches its walk, with no sliding.
