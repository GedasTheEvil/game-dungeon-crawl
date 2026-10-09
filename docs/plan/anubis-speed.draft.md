# Anubis guard walks faster

Status: draft 2026-10-09 (decided, not implemented). From the user: the Anubis guard walks 20% faster than the
player's normal walk. Before [monster-climbers](monster-climbers.draft.md).

## Today

* A monster's `speed` (`MonsterKind`, `src/world/monster_kinds.cpp`) is `MONSTER_SEEK_STEP` (0.0042 tiles) per
  16 ms tick (`UPDATE_TICK_MS`, `Monster::Seek`): one speed point is about 0.26 tiles/s. The player walks
  `WALK_SPEED` 1 tile/s (sprint x3).
* The Anubis guard (`MonsterAnubis`): speed 3, 0.79 tiles/s, 79% of the player's walk. The Anubis boss
  (`MonsterAnubisBoss`): speed 4.5, 1.18 tiles/s.

## Decided (2026-10-09)

* **Anubis guard:** speed 3 → 4.6, 1.21 tiles/s: 120% of the player's walk. The player outruns it only by sprinting.
* **Anubis boss:** speed 4.5 → 5, 1.31 tiles/s, so the boss stays a little faster than its guard.

## Open (for the implementer)

* Threat: a faster guard is harder to kite with the bow. Check its `threat` (8) and the boss's (15), and re-run
  `./levelcheck levels/lvl*` (no warnings).
* The walk clip: at 1.5x the speed the feet may slide; speed up the clip with the walk if needed.
* Balance: the "about 15 blows" note on the guard ([monster-balance](monster-balance.draft.md)) assumed the old
  speed; the sim pass checks it.
* Tests: a unit test or a scenario that times the guard's walk over a few tiles.
