# Monsters rise faster

Status: draft 2026-10-10, ready (needs nothing first). From the user: monsters "rise" too slowly. Jumping on a
cobra or a mummy, the player has nearly killed it by the time it is up. The mummy rises in 1 s, the cobra in 0.75 s
(user, 2026-10-10; first asked 70% of the old time). The monster must get a chance to hit the player at least once.

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

* Rise time: **mummy 1 s**, **cobra 0.75 s** (giant cobra: same as the cobra). Only the Rise state changes; the
  other clips keep `CLIP_SPEED`.
* The two clips need different speeds, so one shared rise speed does not do: give the rise a time per kind or per
  model (e.g. `riseMs` on `MonsterKind`) and derive the clip speed from it and the clip's frame count. Mummy:
  25 frames in 1 s, speed ~62.5. Cobra: 23 frames in 0.75 s, speed ~77.
* `AdvancePlayback` steps every `FRAME_STEP_MS` 100 ms, so a rise ends on a 100 ms step: 0.75 s comes out as 0.8 s.
  For the exact time, drive the rise by elapsed ms (wake time + `riseMs`) instead of the stepped clip; the
  implementer picks. 0.8 s is acceptable.
* "A chance to hit at least once": after the faster rise, a mummy / cobra woken by a jump attack still has to land
  one attack before the player kills it. Check it on the sim: player jumps on it with a typical weapon of the levels
  it appears in, keeps hitting; the monster attacks (or spits) at least once. If not, pick one (implementer's choice):
  * the first attack comes right after the rise (no attack cooldown left to wait for),
  * less damage taken while rising (e.g. half),
  * or a faster rise still (keep 1 s / 0.75 s unless needed).

## Bosses

No boss has a mummy or cobra as `kin` (`src/world/monster_kinds.cpp`), so no boss change. If one gets a rise clip
later, it rises at least as fast.

## Open (implementer's choice)

* Which fallback if one hit is not reached (see above).
* Exact 0.75 s (time-driven rise) or 0.8 s on the 100 ms step.
* Whether the wake sound still fits the shorter clip.

## Tests

* Unit test (`tests/unit/`, e.g. `model_info_test.cpp` or `sim_test.cpp`): a woken mummy stops `Rising()` after
  ~1 s (not ~1.8 s), a cobra after ~0.75 s (0.8 s on the step); the walk clip speed is unchanged.
* Unit / sim test: jump attack on a mummy and on a cobra, the monster hits the player at least once before it dies.
* Visual check: `make test SCENARIO=...` with a mummy and a cobra waking, the rise looks right at the new speed.
