# Sunlight mirrors

Status: draft 2026-10-10, ready (needs nothing first). From the lore review, picked by the user.

## Idea

A shaft of sunlight falls into the tomb. Bronze mirrors on stands redirect it; when the beam hits a sun disc on a
gate, the gate opens. A light puzzle beside keys, levers and riddles.

* New objects (glyphs: pick free ones): **light shaft** (a source cell, a beam straight down or sideways), **mirror**
  (interact to turn it: 4 states, e.g. reflect up/down/left/right in the level's 2D plane), **sun disc** (on a gate or
  as a gate type: opens while lit, stays open once lit).
* The beam travels through open cells, stops at walls, is blocked by the player? (default: no). Drawn as a bright
  dusty volume beam with motes.
* Optional: the beam burns mummies standing in it (with [fire-damage](fire-damage.draft.md) if implemented: fire
  damage per second).
* Level format: new object types (`docs/levels.md`); `levelcheck` must model the beam to prove the level can be
  finished (mirror states are player-set: search over states).
* Journal note: "The priests knew how to bring the sun underground. So did a certain archaeologist with a staff."

## Tests

* Unit: beam path through mirrors, gate opens when lit, checker solves a mirror level.
* A hand-made level using it (a campaign level from ~lvl8 on), no checker warnings.
* Scenario with a screenshot: lit beam, mirror turned.
