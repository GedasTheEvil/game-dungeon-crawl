# Stage 9a: the game's random streams

Status: implemented 2026-09-30 (`964ee27`), verified in play 2026-09-30. Stage 9 of the [code structure
review](../code-structure-review.draft.md); the movement part: [fixed-timestep.md](../fixed-timestep.md).

## Why

Loot, the riddle deck, the blood splashes and the clips' start frames all drew from the C library's `rand()` (and
`random()`), which the game never seeded: every launch rolled the same loot. Scenarios seeded only `srand`, so the
`random()` blood stayed unseeded, and every splash shifted the loot rolls after it, which is why stage 4 could only
animate the monsters in view.

## Done

* `src/world/rng.h`: `level_gen`'s splitmix64 `Rng`, shared (levelgen output unchanged).
* `GameState::random` (`GameRandom`): `gameplay` (loot via `RollChestLoot` / `RollMimicLoot(Rng&)`, the riddle
  deck) and `effects` (`CharacterModel::SpawnPlayback(Rng&)`). A scenario seeds both at each `level`
  ([../testing.md](../../testing.md)); the game seeds them from the clock.
* `ParticleSystem` has its own `Rng` and `Splash(extent)`; the player and monster hits use it.
* `Dungeon::AnimateMonsters` animates every active monster: one killed out of view finishes dying, and a killed
  mimic leaves its chest even if the player looks away.
* No `rand()` / `random()` / `srand` left in `src/`.
* `chest_pickup`, `status_box` (seed 4) and `mimic` (seed 15) take seeds that roll what they test; `mimic` expects
  the mimic's potion at least once (picking its chest up rolls bonuses too). `tests/unit/loot_test.cpp`.
