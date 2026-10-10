# Monsters rise faster

Status: draft 2026-10-10, ready (needs nothing first). From the user: monsters "rise" too slowly. Jumping on a
cobra or a mummy, the player has nearly killed it by the time it is up. Rise in 70% of the current time; the monster
must get a chance to hit the player at least once.

## Today

* A woken mummy (`Locomotion::Entombed`) or cobra / giant cobra (`Locomotion::Coiled`) plays its `_rise` clip once
  (`ModelState::Rise`, `Monster::wake`, `src/entities/monster_ai.cpp`). Until it ends (`Monster::Rising`) the
  monster does nothing else (`src/world/dungeon_monsters.cpp`, `mon.Lurk(...) || mon.Rising()`), but it takes hits.
* Every character clip plays at `ModelInfo::CLIP_SPEED` 35 (`src/entities/model_info.h`): 0.04 x 35 = 1.4 frames
  per `FRAME_STEP_MS` 100 ms, 14 frames/s (`AdvancePlayback`).
* `models/monsters/mummy_rise.md3`: 26 frames, ~1.8 s. `cobra_rise.md3`: 24 frames, ~1.65 s. The giant cobra uses
  the cobra model. A summoned mummy climbing out of its coffin (`Summon::Coffin`) plays the same clip.
* The rise lift in `monster.cpp` (`Progress(state, playback)` for `ModelState::Rise`) follows the clip, so it speeds
  up with it.

## Idea

* Play the rise clip at 35 / 0.7 = **50** (20 frames/s): mummy ~1.25 s, cobra ~1.15 s. Only the Rise state; the
  other clips keep `CLIP_SPEED`. E.g. a `RISE_CLIP_SPEED` next to `CLIP_SPEED`, picked in `ModelInfo::Advance`
  when the shown state is `ModelState::Rise`.
* "A chance to hit at least once": after the faster rise, a mummy / cobra woken by a jump attack still has to land
  one attack before the player kills it. Check it on the sim: player jumps on it with a typical weapon of the levels
  it appears in, keeps hitting; the monster attacks (or spits) at least once. If not, pick one (implementer's choice):
  * the first attack comes right after the rise (no attack cooldown left to wait for),
  * less damage taken while rising (e.g. half),
  * or a faster rise still (keep the 70% unless needed).

## Bosses

No boss has a mummy or cobra as `kin` (`src/world/monster_kinds.cpp`), so no boss change. If one gets a rise clip
later, it rises at least as fast.

## Open (implementer's choice)

* Which fallback if one hit is not reached (see above).
* Whether the wake sound still fits the shorter clip.

## Tests

* Unit test (`tests/unit/`, e.g. `model_info_test.cpp` or `sim_test.cpp`): a woken mummy stops `Rising()` after
  ~1.25 s (not ~1.8 s), a cobra after ~1.15 s; the walk clip speed is unchanged.
* Unit / sim test: jump attack on a mummy and on a cobra, the monster hits the player at least once before it dies.
* Visual check: `make test SCENARIO=...` with a mummy and a cobra waking, the rise looks right at the new speed.
