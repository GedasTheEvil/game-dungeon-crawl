# Sub-cells: player movement and drawing

Status: solved 2026-11-03 (play-tested by the user). Was: implemented 2026-11-03.
Was: draft 2026-11-02, needed [level-subcells](level-subcells.md) and [floor-queries](floor-queries.md)
first. From the user.

Implementer's choices:

* The floor (`Dungeon::floorBelow`): the highest solid top at or under the feet, down to the bottom of the cell
  under them. A whole solid cell counts under the body's centre only (as before: the walk off a ledge, the pits and
  the checker's jumps stay as they were); a solid sub-cell counts under any part of the body, 0.01 in from its sides.
  So a jump lands on a step once a foot is over it, and walking off a step the trailing foot carries the player
  until it is off too.
* `FALL_MAX_STEP` stays 0.3 (the fall speed unchanged; there is no fall damage): instead of a smaller step, the
  landing looks a whole cell down from where the tick starts, so a tick cannot skip a step's top (unit test).
* The body (`Dungeon::bodyBlocked`): whole solid cells still stop the feet only, as before; solid sub-cells stop the
  body from the feet to the head (`PLAYER_BODY_HEIGHT`, 0.375), from the centre to the leading side. The side the
  body leaves is not asked, so the player can always step back out. A gust's push and a charge's shove use it too.
* Head room (`Dungeon::ceilingAt`): a solid cell over the centre or a solid sub-cell over the body stops a jump's
  rise, with a knock (`WorldSound::HeadBump`, `characters/archeologist_bump.wav`, `tools/audio/weapon_sounds.py`).
  From the floor a jump never reaches a ceiling a cell up (`movement.h`), so only shapes (or a jump off a step under
  rock) bump.
* The jump ends on a step higher than its take-off; over a lower floor it ends at the take-off height and falls on,
  as before.
* A step in half water or sand stands out of it: no wading, sink, splash or sand slow on it (`playerOnStep`).
* `isFloor` also counts a shaped cell with its top row all solid (`topRowSolid`), for monsters and the checker too.
* Ladders, gates and doors in shaped cells are not stopped by the game; a ladder beside a step works by the body
  rules (climb above the step's top, step over). The checker's warning stays with
  [subcell-checker](../subcell-checker.draft.opus.md); not unit-tested.
* Drawing (`Dungeon::drawShapeBlocks`): the front of each run of solid sub-cells along a row in one face, the cell's
  wall texture (its back wall's pattern continued), tops in its floor texture, undersides in its ceiling texture, sides
  in the wall texture; only faces next to an open sub-cell. In half water the bottom row reaches down to the basin
  floor; the water's surface inside a block is hidden by it.
* Objects, fixtures and placed props of a shaped cell are drawn on `floorHeight(t, 0.5f)`. The scatter puts no random
  prop, torch or decal in a shaped cell; placed decals and torches are drawn as placed.
* Arrows, sling stones, thrown sticks and javelins and the monsters' shots stick or splat at a solid sub-cell
  (`subcellAt`). A sunbeam runs along the cell's middle row or column and stops at the first solid sub-cell on it
  (`beamReach`, drawn up to it); the checker's beam search shares the rule (`BeamBlock` takes the way).
* `tests/levels/subcells` rebuilt (from its `.txt`): a hall two cells high, so the steps have head room.

The player walks, jumps and falls against the solid sub-cells of a shaped cell, and the game draws them.

## Player

* Collision against solid sub-cells, with the hitbox's half width (`Player::HalfWidth`, the green box of the F3 view;
  walls and gates too, no longer `PLAYER_BODY_HALF_WIDTH`, 0.25 tiles) and height (about 0.375 tiles). No auto step-up: walking into a quarter step stops the player, a jump gets them onto it (from
  the user: parts you need to jump on).
* Landing: a fall stops at the floor height under the player. Keep `FALL_MAX_STEP` below a quarter tile (today 0.3),
  so one tick never skips a step's floor; check the fall speed and fall damage stay as today.
* Head room: a jump under a low ceiling stops at it (a bump sound); walking into an opening lower than the player is
  blocked (crouching through comes with [crouch](crouch.md)).
* Walking off a step drops the player to the floor below: a short fall, no damage.
* Ladders, gates and doors need a cell open from floor to ceiling; climbing off a ladder onto a step works if the
  step's top is free.
* Update `src/world/movement.h`: the `static_assert`s say a jump clears a quarter step and not a half, and still
  clears a one-cell gap.

## Drawing

* `Dungeon::Draw`: solid sub-cells drawn as blocks with the cell's wall texture (the zone look), their tops as floor,
  their undersides as ceiling; merge neighbours into one box where it is simple.
* Decor, torches and decals of a shaped cell sit on its floor height or are left out where they do not fit (the
  scatter skips them; a placed one the checker warns about in [subcell-checker](../subcell-checker.draft.opus.md)).
* Beams (`traceBeams`), arrows and thrown items: a solid sub-cell stops them where it is. Water fills a shaped cell
  above its floor.

## Tests

* Unit: blocked by a quarter step, a jump lands on it, cannot jump onto a half; a fall lands on a step; a jump stops
  at a low ceiling; an arrow stops at a block.
* Scenario `tests/scenarios/solved/subcell_physics.txt` on `tests/levels/subcells`: screenshots of steps, a stair, a ledge lip,
  a low ceiling.
* Play script `tests/play/subcell-physics.txt` (removed once solved).
