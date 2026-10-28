# A climber crossing a ladder cell shows the climb clip

Status: implemented 2026-10-28 (open point: the whole ladder cell switches, no flicker); was draft 2026-10-28, needs nothing first. From the user: a climber followed me along a straight corridor with
one ladder cell in it (no floor under it). It walked across that cell with its walk clip, as if on air. It should
switch to the climb clip there.

## Today

* A climber's path ([monster-climbers](monster-climbers.md)) counts a ladder cell as a cell it can stand on
  (`ClimbMap::Stands`), so along a row `Dungeon::followPath` walks it across a ladder cell with no floor under it
  (`WalkPath`, `ModelState::Move`).
* `Dungeon::UpdateMonsters` (`dungeon_monsters.cpp`) already marks it `OnRungs` there (a ladder, no floor under it), so
  it cannot bite. But the climb clip (`ShowsClimb`: back to the camera, `ClimbDepth`, `showClimb`) only plays while
  `state == ModelState::Climb`, which only `StartClimb` / `UpdateClimb` set. A walk across keeps the walk clip.
* The player does show the climb across such a cell: `PlayerOnLadder` (within `LADDER_REACH` of `LADDER_GRIP_X`),
  frame from `ClimbPhase` (`mapY + CLIMB_SIDE_RATE * mapX`), so the hands move on a sideways climb too.

## Idea

* A live climber `OnRungs` shows the climb clip, also while it moves sideways on the rungs: `WalkPath` (and any other
  horizontal step) enters `ModelState::Climb` instead of `Move` while on the rungs.
* `Monster::showClimb` takes the frame from the height only, so a sideways move would freeze the clip. Add the
  horizontal part as the player does (`CLIMB_SIDE_RATE`, move the constant to `gameplay_config.h` or
  `render_config.h` to share it): phase from `row + climb.y + CLIMB_SIDE_RATE * x`.
* Back on a cell with a floor under it, the walk clip again (the facing turns back from 180° to the walk direction).

## Open (for the implementer)

* Where on the cell the clip switches: the whole ladder cell (today's `OnRungs`, simplest) or, as the player, only
  near the grip (`LADDER_REACH`). Pick the one that does not flicker or snap visibly.
* Non-climbers never step onto such a cell (`walkerBlockedAt`: no floor under it): no change for them.
* Tests: a unit test (sim harness, `tests/unit/climb_test.cpp`): a rat chasing the player along a row over a ladder
  cell with no floor under it shows `ShowsClimb()` on that cell and not before or after. A scenario in
  `tests/scenarios/` with a screenshot of a climber halfway across, to check the clip and the depth by eye.
* Related: [climber-off-the-rungs](../climber-off-the-rungs.draft.opus.md) also changes what a climber does `OnRungs`;
  if both are implemented together, keep one notion of "on the rungs".
