# Each monster's walk clip keeps pace with its walk

Status: solved 2026-10-29 (draft 2026-10-28). From the user: the walk speeds went up, but for some monsters the legs
move too slowly. The Anubis guard walks at a good speed, but its walk clip is slow. The scarab could walk even faster,
and its legs move too slowly as well. Each monster needs its own walk clip pace, not a global one.

## Today

* `MonsterKind::walkClipSpeed` (`src/world/monster_kinds.h`, default `ModelInfo::CLIP_SPEED` 35) sets the `Move`
  clip's pace ([anubis-walk-clip-speed](anubis-walk-clip-speed.md)). `AdvancePlayback` adds `speed / 25`
  frames every `FRAME_STEP_MS` 100 ms, so 35 is 14 fps.
* Only six kinds set it: the Anubis guard 70, the Anubis boss 76, the camel spider and the Mother of Webs 60, the
  baboon 84 and Babi 50. All others play at 35, whatever their `speed`.
* A monster walks `speed * MONSTER_SEEK_STEP` per 80 ms step: about `0.0525 * speed` tiles a second (the guard's 23:
  1.21 tiles/s). The clip pace is a fixed number, so it does not follow the walk: not the kind's `speed` changes, not
  `waterSpeed` in water, not a boss's faster walk unless someone sets both.
* The guard at 70 (28 fps over 26 frames) makes about 1.1 tiles per stride cycle. The user still finds it slow.

## Idea

* The kind's baseline is its **stride**: tiles covered by one cycle of its walk clip (`MonsterKind::walkStride`), a
  property of the model's legs and scale, not of its speed. The clip pace follows the actual walk:
  `fps = tiles per second / walkStride * walk clip frames`. So a faster `speed`, a slower `waterSpeed` or a boss at a
  bigger scale (a longer stride) all keep the feet planted without retuning a number.
* `walkClipSpeed` goes; a unit test checks that every walker with a walk clip has a stride and that its pace follows
  its speed (half the speed, half the fps).
* Measure each walker's stride from its model (how far a planted foot travels back in one cycle at its `scale`),
  then tune in play until the feet do not slide. Start with the guard: shorten its stride until the user's "slow
  legs" is gone. The model viewer has `+` / `-` speed keys to judge a clip by eye.
* A clip that is not a stride (a worm's crawl, a cobra's slither, a hop) gets a stride too: the distance one cycle
  should cover. Monsters that do not walk (plants, the lotus, egg clusters) keep `CLIP_SPEED`.
* **Scarab walks faster:** `speed` 4 → about 6 (0.21 → 0.32 tiles/s), tuned in play; the stride makes its legs
  follow. Its kin keep their order: the giant scarab (3) and Khepri (`MonsterBossScarab`, 3.5) go up by the same
  share, so the boss stays faster than the giant. Re-run `bin/levelcheck content/levels/lvl*` (a faster scarab may
  score higher) and fix the levels it flags.

## Open (for the implementer)

* Where the pace is computed: `Monster::Animate` passes the current walk rate (with `waterSpeed`) to
  `ModelInfo::Advance` instead of `walkClipSpeed`. A step the monster stands in (`WalkPath` blocked, `Stand`) is
  idle, not a walk.
* Fleeing, charging, kiting backwards: same walk rate, same rule. A charge with its own clip keeps its own pace.
* Play-test: `tests/play/walk-clip-stride.txt` summoning the guard, the scarab and a few others in a row, or
  `bin/game --level N` on a level with guards and scarabs. Ask the user which other monsters look off.

## Implemented (2026-10-29)

* `MonsterKind::walkStride` (model units: the largest extent of frame 0 is 1) replaces `walkClipSpeed`. Kinds on one
  model share it (constants in `monster_kinds.cpp`), so a boss's bigger scale is a longer stride. Flyers and monsters
  that do not move have 0: their clip plays at `CLIP_SPEED`.
* `Monster::WalkClipSpeed`: tiles a second of its last step (`walked`, set by `Seek`, `Flee`, `BackOff`; with
  `waterSpeed`), the rush's speed while charging, divided by the stride in tiles (`scale`, toon size), times the clip's
  frames (`ModelInfo::WalkSpeed`). Steps run every `MONSTER_STEP_PERIOD_MS` (80: the 70 ms step timer on 16 ms ticks).
* A `Seek` blocked by the terrain (a wall, a ledge, light for a shunner) stands the walker in its idle clip.
* A fast loop no longer snaps to frame 0 at its end (`NextFrame`, `BlendOf`): it carries on into the next round, or
  a fast clip would drop part of each round.
* Strides measured with the new `tools/blender/md3_stride.py`: the baboon and Babi measure exactly what they were tuned
  to by eye (their clip speeds 84 and 50), which backs the method. The guard (0.78), the scorpions (0.26), the camel
  spiders (0.54), the scarabs (0.29), the crocodiles (0.12) and hippos (0.19) slid a lot: their legs now play 2-9 times
  faster (the guard 85 fps, the scarab about 150, the scorpion 127). The mummies (0.98, 0.93) slow down to half.
  Clips that are not a stride (rat gallop, worm crawl, cobra slither, ushabti waddle, cursed chest hop) keep about the
  pace they had.
* Scarab `speed` 4 → 6, giant scarab 3 → 4.5, Khepri 3.5 → 5.25. `bin/levelcheck content/levels/lvl*`: no warnings.
* Tests: `tests/unit/walk_stride_test.cpp` (every walker has a stride, one per model; a scorpion's pace in half water
  is half its pace on land, one stride per round), `model_info_test.cpp` (`WalkSpeed`, the loop carrying on).
* Play-test: `bin/game --play tests/play/walk-clip-stride.txt` (tests/levels/walk_stride_parade: a guard, a scarab, a
  scorpion, a camel spider, a giant scarab, a giant scorpion in turn, a half-water pool by the start). Tell which
  monsters still look off: a stride constant scales its legs' pace (smaller: faster legs).
* Play-test 1 (2026-10-29): the giant scarab perfect, the scarab OK; the guard, the camel spider and the scorpions
  shook. A step lands on the first 16 ms tick past `FRAME_STEP_MS`, at 112 ms, and `BlendOf` blended over 100 ms: the
  pose held still the last 12 ms of every step, unseen at a frame or two a step, a shake at 8. `BlendOf` and
  `WalkSpeed` now use the real `FRAME_STEP_PERIOD_MS` (112); clip timings are as before (the legs were 11% slow).
* Play-test 2: still a shake. The body moved by a whole walk step every 80 ms (drawn at its sim x): with the legs in
  pace, the planted foot shook against it. A walk step now glides over the length of the last one
  (`Monster::stepBy`, `LocalX`), and `AnimPlayback::stepMs` measures each clip step, so late ticks do not hold either.
* The user picked loop times by eye in the model viewer (Light: toon): scorpion 0.95 s, Anubis 0.54, mummy 1.64,
  cobra 1.31, baboon 2.05, camel spider 1.05, bat 0.84 (a flyer: 0.86 at `CLIP_SPEED` already, kept). Their strides
  give that at the base kind's walk on land; kin on a bigger scale take longer rounds (the scorpion queen 6.1 s, Apep
  3.8 s, the High Priest 3.0 s).
* By eye too: the scarab 0.67 s (its own stride now; the giant scarab and Khepri keep the measured 0.29, rated
  perfect), the crocodile on land 1.31 s (Sobek 2.5 s). Kin on one model need not share a stride any more.
