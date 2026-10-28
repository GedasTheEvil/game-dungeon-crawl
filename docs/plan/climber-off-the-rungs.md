# A climber in reach leaves the rungs to fight

Status: draft 2026-10-28, implemented 2026-10-28, needs nothing first. From the user: when a monster climbs after me, I can step off the
ladder and keep hitting it, and it stays on the ladder. It should find ground to attack me from.

## Today

* A climber ([monster-climbers](solved/monster-climbers.md)) cannot attack on the rungs (`Monster::OnRungs`, the
  `!mon.OnRungs()` check at the end of `Dungeon::updateWalker`).
* The exploit: the player climbs, steps off the ladder onto a floor cell beside it and turns. The climber finishes its
  climb on the player's row (`climbEnd`), so it is on that row and in reach. `Dungeon::followPath` then hands it to the
  walker's rules (`mon.sameRow(mapY) && (dir == 0 || walkerReaches(...))`). `attackDirection` is 0 (in reach), so
  `Monster::Seek` does not move it, and the attack is blocked by `OnRungs`. It hangs on the rungs, unable to bite,
  while the player hits it.
* Likely the same at a ladder top: the player steps off just beside the top rung, the climber stops on a rung cell
  without a floor under it.

## Idea

* A climber on the rungs never takes the walker's rules. While `OnRungs()`, `followPath` keeps it on the path: it steps
  off the ladder onto the nearest cell it can stand on and attack from (`ClimbMap::Stands`, floor under it), towards
  the player, then the walker's rules as today (chase, bite).
* If the player stands on the only floor cell next to the rung (no other cell to step onto): the climber steps into
  the player's side as a walker does when it bites (pushes up against them), off the rungs, and bites. The point:
  standing next to a ladder is no safe spot.
* Same for every climber and the climber bosses (Scorpion Queen, Anubis boss): one code path.

## Open (for the implementer)

* Reproduce first: a unit test on the sim harness (`tests/unit/climb_test.cpp`, `sim_world.h`): a rat climbs after the
  player, the player steps off sideways at a mid-shaft ledge and at the top; today the rat stays `OnRungs` and never
  bites. After the fix it leaves the rungs and bites within a few seconds.
* Whether the step off counts as the climb's end, or a short walk off the ladder column (`WalkPath`) once the climb
  ended. Check the climb clip does not snap.
* Check the player cannot still kite it: stepping back onto the ladder as the climber steps off, then off again.

## Done (2026-10-28)

* `Dungeon::rungsExit`: a climber on the rungs with the player on its row and in reach walks off them
  (`WalkPath`), to the floor cell on the player's side first, else the other side; then the walker's rules bite.
  One code path for every climber and climber boss. The climb ends as before (`climbEnd`); the step off is a walk
  with the climb clip while on the rung cell (`ShowsClimb`), so no snap.
* Kiting: the player cannot attack from the rungs ([no-attack-on-ladder](solved/no-attack-on-ladder.md)), so going
  back onto the ladder buys nothing. A climber does not follow onto the player's rung cell: it stands against the
  ladder, and once the player steps off either side it crosses the rungs and bites.
* Tests (`tests/unit/climb_test.cpp`, failing before the fix): the top of the ladder (`climb_up`), a mid-shaft ledge
  (`tests/levels/climb_ledge`), on and off the ladder.
* Play-test: `bin/game --play tests/play/climber-off-the-rungs.txt`.
