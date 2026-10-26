# Model frame interpolation

Status: draft 2026-10-24, from the user ("the model has key frames exported from Blender, but the animation looks
chopped; interpolate between 2 frames for smoother movement"). Needs nothing first. Implemented 2026-10-26, solved (play-tested)
2026-10-26: `bin/game --play tests/play/model-frame-interpolation.txt`.

## Done

* `AnimPlayback::step` (set by `AdvancePlayback`) and `BlendOf` (`src/entities/model_info.cpp`, unit tested): the
  render pose. A frame set by hand (`step` 0: the climb, the burrow rise and dive, a grab's held frame, the bow's
  draw) blends its own fraction, so the dive runs backwards smoothly too.
* GPU: the lighting shader's `aNextPos` / `aNextNormal` (attributes 6, 7, bound before the link) and `uBlend`;
  `AnimatedModel::Show` feeds them. The ink pass is screen space (depth and colour), nothing to do there. Without the
  shader (lighting failed, or a draw outside the scene's `Lighting::begin`/`end`, e.g. the journal) a model draws
  key frame a, as before.
* Options > Display > Animation rate (`Key frames`, 24, 30, 60), `animation_rate` in `[display]`. The Display rows
  are a little lower (`OPTION_H` 6.3) to fit eight.

## Why it looks chopped

Two causes, both in the engine, not in the models:

* `AdvancePlayback` (`src/entities/model_info.cpp`) moves `AnimPlayback::frame` only every `FRAME_STEP_MS` (100 ms),
  so the pose changes at most 10 times a second, whatever the render rate.
* `AnimatedModel::Show` (`src/graphics/animated_model.cpp`) truncates `frame` to an int: the fraction is thrown
  away, so a clip at speed 25 (1 frame per step) shows each exported frame for a full 100 ms, a slower clip
  holds a frame for several steps.

## Goal

Vertex interpolation, as Quake 3 does with MD3: draw `lerp(frame a, frame b, t)` for positions and normals, with
`a = floor(pos)`, `b` the next frame, `t` the fraction, `pos` computed from the game clock at draw time. Movement is
smooth at any frame rate, no re-export, no change to the Blender pipeline.

## How

1. Continuous playback position. Keep the sim side (`AdvancePlayback`, steps, `ModelInfo::Done`/progress, hit
   timing) as it is, so gameplay and tests don't change. Add a render-only position: the stepped `frame` plus the
   part of the next step already passed (`(now - stepStart) / FRAME_STEP_MS * step`), clamped so it never runs past
   what the next step will set (one-shot: the last frame; loop: wraps to 0).
2. Next frame: a looping clip goes `N-1 → 0` (loops are exported without the repeated end frame, docs/modeling.md),
   a one-shot clip holds `b = a` at its last frame.
3. Blend on the GPU. The lighting shader (`src/graphics/lighting.cpp`) reads `gl_Vertex`/`gl_Normal`; add two
   generic attributes (next frame's position and normal, both offsets into the same VBO, like `Show` does now) and a
   `blend` uniform; `mix()` both, normalize the normal. The ink (toon) pass and any other shader that draws models
   needs the same. Without shaders (the fixed-function fallback): CPU lerp into a scratch array, or just draw
   frame `a` as now (implementer's choice; measure the CPU cost on the largest model first).
4. Other users of the frame number stay integer: `Player::shownFrame`, `Item` poses (`item.cpp`), `Vertex`/`YRange`
   of a frame. Check every `static_cast<int>(...frame)` reader.

5. Option: Options > Display, "Animation rate" (from the user): how many poses a second a model shows. Choices:
   `Key frames` (no blending, today's look), `24`, `30`, `60`, default `60`. The render position from point 1 is
   snapped to the chosen rate (`floor(clockMs * rate / 1000)` time steps) before the blend, so 24 gives a
   deliberate stop-motion look and 60 is smooth on a 60 Hz monitor. Above the render rate the cap does nothing.
   Saved with the other settings (`Settings::Display`, `src/state/settings_ini.h`, docs/settings.md); a scenario
   run uses the default. The whole-game cap is a separate plan: [frame-rate-cap](../frame-rate-cap.md).

## Watch out

* Clip switches (move → attack, any → die) still jump. Cross-fading between clips is a possible later step, not part
  of this plan.
* The rise clip played backwards (sand burrowers diving, `Monster::showBurrow`): interpolate in its direction.
* Scenario screenshots are taken at fixed clock values; poses can shift a little. Update the reference shots only
  where the difference is the blend.
* Optional follow-up: with smooth playback, `FRAME_STEP_MS` could drop or clips could be exported with fewer key
  frames. Not part of this plan.

## Test

* Unit: the render position for loop and one-shot clips (wrap, clamp at the last frame, `t` in [0, 1)).
* Play-test: `bin/game --play tests/play/babi-boss.txt` or any level with walking monsters; compare walk and attack
  cycles before and after.
