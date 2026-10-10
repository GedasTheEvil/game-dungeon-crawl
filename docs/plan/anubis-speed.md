# Anubis guard walks faster

Status: implemented 2026-10-10, not play-tested yet (see [Implemented](#implemented-2026-10-10)). From the user: the Anubis guard walks 20% faster than the
player's normal walk. Before [monster-climbers](monster-climbers.md).

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

## Implemented (2026-10-10)

* The speeds in "Today" were wrong: a monster takes a `MONSTER_SEEK_STEP` step each time its 70 ms step timer is due
  (`Monster::StepDue`, in practice every fifth 16 ms tick, 80 ms), not every tick. One speed point is about
  0.05 tiles/s. Measured on the sim harness, the guard at speed 3 walked 0.24 tiles/s (the boss 0.36), a quarter of
  the player's walk, not 79%. So the decided speeds 4.6 and 5 would have left both slower than the player.
* Implemented as decided in tiles a second: the guard at **23** (1.21 tiles/s, 120% of the walk), the boss at **25**
  (1.31 tiles/s). That is five times as fast as before for the guard, nearly four for the boss: a real change for
  the finale. The giant rat (24) is the only monster that fast so far.
* Threat: the guard 8 → 9 (only a sprint gets away; the bow cannot keep it off). The boss stays at 15. `levelcheck`:
  no warnings.
* Not done: the walk clip still plays at its old pace, so the feet may slide; check in play
  (`tests/scenarios/summon_effects.txt` and the levels with Anubis guards).
* Tests: `tests/unit/monster_rules_test.cpp` measures the guard's walk over a second on the sim harness. The tests
  that timed the old guard (the spikes, the dart plate, the Anubis boss's blow) now wait for the event instead.
