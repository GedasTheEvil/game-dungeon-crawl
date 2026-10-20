# Sunlight mirrors

Status: draft 2026-10-10, from the lore review, picked by the user. Implemented 2026-10-20, see [Implemented](#implemented); play-test: `bin/game --play tests/play/sunlight-mirrors.txt`. Solved 2026-10-20 (play-tested); its screenshot scenario to tests/scenarios/solved/.

## Idea

A shaft of sunlight falls into the tomb. Bronze mirrors on stands redirect it; when the beam hits a sun disc on a
gate, the gate opens. A light puzzle beside keys, levers and riddles.

* New objects (glyphs: pick free ones): **light shaft** (a source cell, a beam straight down or sideways), **mirror**
  (interact to turn it: 4 states, e.g. reflect up/down/left/right in the level's 2D plane), **sun disc** (on a gate or
  as a gate type: opens while lit, stays open once lit).
* The beam travels through open cells, stops at walls, is blocked by the player? (default: no). Drawn as a bright
  dusty volume beam with motes.
* Optional: the beam burns mummies standing in it (with [fire-damage](fire-damage.md) if implemented: fire
  damage per second).
* Level format: new object types (`docs/levels.md`); `levelcheck` must model the beam to prove the level can be
  finished (mirror states are player-set: search over states).
* Journal note: "The priests knew how to bring the sun underground. So did a certain archaeologist with a staff."

## Tests

* Unit: beam path through mirrors, gate opens when lit, checker solves a mirror level.
* A hand-made level using it (a campaign level from ~lvl8 on), no checker warnings.
* Scenario with a screenshot: lit beam, mirror turned.

## Implemented

Implemented 2026-10-20. Play-test: `bin/game --play tests/play/sunlight-mirrors.txt` (lvl21, in the hall of the red
lock), or `bin/game --level tests/levels/mirrors` (the bare puzzle).

Choices for the open points: the sun disc is a fixture with a lock colour (a lever the beam pulls: it stays lit, its
gates stay open), not a gate type; the player does not block the beam; the beam burns mummies (fire damage) and keeps
camel spiders out as a torch does.

* **Tiles** (`level.h`, `tile_defs.cpp`, `src/world/sun_beam.h`): `LightShaft` (17, `l`; attribute its way: 0 down,
  1 left, 2 up, 3 right), `Mirror` (18, `(`; value the way it points), `SunDisc` (19, `a`, a wall type; attribute the
  lock colour, value 1 once lit). A mirror sends any beam that falls on it the way it points; pointing the way the beam
  goes it stands edge-on (the beam passes), pointing back it faces the beam and catches it. Interact (`Turn`) turns it
  a quarter turn clockwise. Rock, deep water and gates not fully open stop the beam.
* **The beam** (`traceBeams`, `Dungeon::updateSunBeams` in `dungeon_sun.cpp`): traced every tick; a sun disc in it is
  lit, its gates open (with the cut-scene), "The Aten blazes" (the Sun Disc is already the fire-guard amulet's name).
  A mummy or the mummy priest awake in a beam burns (`MonsterKind::sunBurns`, `SUN_BURN_FIRE`: 15 HP a second while in
  it). Sunlit cells count as light for `Dungeon::Lit` (the camel spiders).
* **Drawing** (`dungeon_render_sun.cpp`, `SunBeam::ray` / `glow` in `graphics/fire.cpp`): a white core in a warm glow
  with dust drifting along it, 0.3 tiles in front of the back wall at half a tile's height; a glow where it comes in,
  at each mirror and where it falls on rock; a light every other cell. The mirror's face is drawn halving the angle
  between the beam in and out (from above when no beam falls on it), swinging over `MIRROR_TURN_MS`; a gold pointer on
  its hub shows its way. A lit disc glows, flaring as it lights.
* **Models** (`mechanism.py`): `mirror_stand` (a sandstone foot, a papyrus column, a bronze fork), `mirror` (a bronze
  oval, polished face, gold rim), `mirror_pointer`, `sun_disc` (the Aten between two cobras, rays, the lock gem; four
  colour textures). Sounds `mirror_turn.wav`, `sun_disc.wav` (`mechanism_sounds.py`).
* **Checker** (`level_check.cpp`): turning a mirror (`PathMove::Turn`) lights every sun disc a beam can reach with the
  mirrors the player reaches from there, the colours kept, set as needed (`sunlitCells`: a search over the beam's
  cells and ways, a free mirror sending it any way but back); a disc lit as the level starts opens its gates from the
  start. Warnings: a light shaft without rock behind it, a mirror in front of an inscription, a sun disc no beam can
  ever reach, one lit as the level starts. 0.8 difficulty per turn on the path. `levelcheck --script` turns the
  mirror with the interact key and sets the others the beam needs with the new scenario command `mirror COL ROW WAY`.
* **Level**: lvl21's red lever is now a red sun disc at the west end of its hall; a one-cell crack over the hall
  (col 12) holds the light shaft, the mirror under it faces the sun: three turns send the beam west, past the hall's
  mummy. Draft map symbols, editor hints, the field note "Sun mirrors" (with the draft's line).
* **Tests**: `tests/unit/sun_mirrors_test.cpp` (`tests/levels/mirrors`, `mirrors_mummy`), scenario
  `tests/scenarios/solved/sun_mirrors.txt` (screenshots).
* Not done: mirrors in the other desert levels (22-25 keep their levers and keys); no editor icons for the three
  tiles (flat colours).
