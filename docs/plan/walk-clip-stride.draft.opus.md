# Each monster's walk clip keeps pace with its walk

Status: draft 2026-10-28, needs nothing first. From the user: the walk speeds went up, but for some monsters the legs
move too slowly. The Anubis guard walks at a good speed, but its walk clip is slow. The scarab could walk even faster,
and its legs move too slowly as well. Each monster needs its own walk clip pace, not a global one.

## Today

* `MonsterKind::walkClipSpeed` (`src/world/monster_kinds.h`, default `ModelInfo::CLIP_SPEED` 35) sets the `Move`
  clip's pace ([anubis-walk-clip-speed](solved/anubis-walk-clip-speed.md)). `AdvancePlayback` adds `speed / 25`
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
