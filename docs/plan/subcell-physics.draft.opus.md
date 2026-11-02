# Sub-cells: player movement and drawing

Status: draft 2026-11-02. Needs first: [level-subcells](level-subcells.draft.opus.md) and
[floor-queries](solved/floor-queries.md). From the user.

The player walks, jumps and falls against the solid sub-cells of a shaped cell, and the game draws them.

## Player

* Collision against solid sub-cells, with the body half width (`PLAYER_BODY_HALF_WIDTH`, 0.25 tiles) and height
  (about 0.375 tiles). No auto step-up: walking into a quarter step stops the player, a jump gets them onto it (from
  the user: parts you need to jump on).
* Landing: a fall stops at the floor height under the player. Keep `FALL_MAX_STEP` below a quarter tile (today 0.3),
  so one tick never skips a step's floor; check the fall speed and fall damage stay as today.
* Head room: a jump under a low ceiling stops at it (a bump sound); walking into an opening lower than the player is
  blocked (crouching through comes with [crouch](crouch.draft.opus.md)).
* Walking off a step drops the player to the floor below: a short fall, no damage.
* Ladders, gates and doors need a cell open from floor to ceiling; climbing off a ladder onto a step works if the
  step's top is free.
* Update `src/world/movement.h`: the `static_assert`s say a jump clears a quarter step and not a half, and still
  clears a one-cell gap.

## Drawing

* `Dungeon::Draw`: solid sub-cells drawn as blocks with the cell's wall texture (the zone look), their tops as floor,
  their undersides as ceiling; merge neighbours into one box where it is simple.
* Decor, torches and decals of a shaped cell sit on its floor height or are left out where they do not fit (the
  scatter skips them; a placed one the checker warns about in [subcell-checker](subcell-checker.draft.opus.md)).
* Beams (`traceBeams`), arrows and thrown items: a solid sub-cell stops them where it is. Water fills a shaped cell
  above its floor.

## Tests

* Unit: blocked by a quarter step, a jump lands on it, cannot jump onto a half; a fall lands on a step; a jump stops
  at a low ceiling; an arrow stops at a block.
* Scenario `tests/scenarios/subcell-physics` on `tests/levels/subcells`: screenshots of steps, a stair, a ledge lip,
  a low ceiling.
* Play script `tests/play/subcell-physics.txt`.
