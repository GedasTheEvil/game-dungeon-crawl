# Player idle: feet planted, a natural standing idle

Status: implemented 2026-11-11, waiting for the user's play-test. Was: draft 2026-11-11, size M, needs nothing first.

## Idea

The player's idle clip (`archeologist.md3`, `idle_poses()` in `tools/blender/models/archeologist.py`) looks wrong: as
if the archeologist hangs on his elbows and swings his legs. The feet slide and swing instead of standing on the floor.
Wanted: both feet planted on the ground for the whole loop, and the body moves the way a standing person idles.

Likely causes (check, don't trust):

* The weight shift is a pelvis rotation (`pelvis` rz, `thigh` rz): the pelvis is the legs' parent, so it swings both
  legs round the hip like a pendulum; the per-frame floor fix (root z) then lifts the body to the lowest foot.
* The arms stay in `READY` (upper arms forward, forearms bent 96°) and only the torso moves, so the still elbows read
  as the support the body hangs from.

## Plan

* Feet fixed in world space for every idle frame: shift the weight by moving the hips (`root_loc` x, a little z),
  and solve thigh / shin / foot angles so each ankle stays where it is (a small two-bone solve in the script, or
  Blender IK baked to the keys). Foot soles stay flat on the floor.
* A human idle on that base, one loop (32 frames, keep the length and frame 0 close to the walk's passing pose):
  * slow weight shift from one leg to the other, the loaded knee straight, the other a little bent;
  * breathing in the chest and shoulders (small);
  * the head looks around a bit (keep the current head turn);
  * the arms hang with the body: elbows follow the chest, a small lag, not fixed in space. Keep the weapon fist
    where the engine finds it (`archeologist.py` header: the most forward corners at 50-85 % of the idle height), and
    keep the hitbox / top height the crouch and `player_view.cpp` read from idle frame 0 about the same.
* Check: a test (unit or scenario) that the foot vertices move less than ~1 cm across the idle frames, and a
  scenario with screenshots of a few idle frames, side view.
* The crouch still (`archeologist_crouch`) shares the idle's breath; the stand-to-crouch clip starts at idle frame 0:
  check both still look right.

## Play-test

* `bin/game --level 1`, stand still: both feet stay on the floor, the body shifts its weight, breathes, looks around.
* No hanging-on-the-elbows look, no swinging legs.
* Walk, stop, crouch, stand up: no pop between the idle and the other clips.
