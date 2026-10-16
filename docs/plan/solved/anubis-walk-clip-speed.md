# Anubis walk clip plays faster

Status: solved 2026-10-16 (play-tested by the user). Follows up [anubis-speed](anubis-speed.md) ("Not done: the walk
clip still plays at its old pace").

From the user (2026-10-13): the Anubis walks fast, but its legs move slowly. The walk clip must play at least twice
as fast as now.

## Today

* Every character clip plays at one pace: `ModelInfo::CLIP_SPEED` 35 (`src/entities/model_info.h`), so
  `AdvancePlayback` (`src/entities/model_info.cpp`) adds 1.4 frames each `FRAME_STEP_MS` 100 ms: 14 fps.
* The Anubis walk (`tools/blender/models/anubis.py`, `walk_poses`): 26 frames, one full stride cycle (left and right
  step), so 1.86 s a cycle. The guard walks 1.21 tiles/s (speed 23), the boss 1.31 (speed 25): about 2.3 tiles each
  cycle, the feet slide.
* `Monster::Animate` (`src/entities/monster.cpp`) calls `type->model.Advance(state, playback, type->riseMs)`; only
  the rise clip has its own pace (`riseMs`).

## Plan

* A new `MonsterKind` field, the walk clip's pace: e.g. `int walkClipSpeed = ModelInfo::CLIP_SPEED;` (the
  `AdvancePlayback` speed of the `Move` clip). Only the `Move` state uses it; the attack (also the ranged clip), die,
  climb and idle clips keep `CLIP_SPEED`, so the bolt's release timing (`ANUBIS_RELEASE`) does not change.
* Pass it through `ModelInfo::Advance` (an overload or a parameter, like `riseMs`). Every other monster keeps the
  default: no visible change for them.
* **Anubis guard** (`MonsterAnubis`): 70 (2x, 28 fps, 0.93 s a cycle). **Anubis boss** (`MonsterAnubisBoss`, kin of
  the guard): 76 (its walk is 25/23 of the guard's, so the same stride). AGENTS.md "Boss abilities".
* If 2x still slides in a screenshot sequence (the planted foot should stay still on the floor), raise both until
  it does not. The user asked for "at least double".
* Keep `CLIP_SPEED`'s comment true ("every character clip" → "the default pace of every character clip").

## Tests

* Unit test (`tests/unit/`): an Anubis guard's `Move` playback advances twice the frames of a mummy's over the same
  GameClock time; its attack playback advances the same as before.
* Play-test command for the user: `bin/game --level N` with N a level that has Anubis guards (glyph `n`, levels 13
  on), or a `tests/play/anubis-walk-clip-speed.txt` script that summons a guard next to the player.

## Implemented

* `MonsterKind::walkClipSpeed` (default 35, `ModelInfo::CLIP_SPEED`), passed to `ModelInfo::Advance`; only `Move`
  uses it. Anubis guard 70, boss 76. Other clips keep `CLIP_SPEED`.
* The play-test found the speeds right.
* Tests: `tests/unit/model_info_test.cpp`; play script `tests/play/anubis-walk-clip-speed.txt`.
