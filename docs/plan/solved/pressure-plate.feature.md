# Pressure plates open gates by weight

Status: solved 2026-11-14 (play-tested by the user). Was: implemented 2026-11-13, waiting for the user's play-test. Was: draft 2026-11-13, size L, needs [mirror-drag](mirror-drag.feature.md) first.

## Idea

A new puzzle: a pressure plate opens a gate while enough weight stands on it. Step off, and the gate closes. The
solution is to drag a heavy object onto the plate. Dragging comes from [mirror-drag](mirror-drag.feature.md).

## Plan

* Pressure plate: a floor cell with a `required weight`. Its gate stays open while the summed weight of the items on
  the plate is at least that value; under it, the gate closes.
* Weights of draggable items (the implementer sets the values): player, rock, giant monster body, monster body
  (any kind), empty chest, small monsters (living or dead, same weight). Items add up. A mix counts, e.g. 1 rock +
  1 chest + 1 small monster.
* Draggable items: the rock (new prop), monster bodies (dead monsters become draggable), and every chest once
  emptied: treasure chest, trapped treasure chest, mimic, cursed mimic.
* Gate: re-skin the existing `Gate` model with a new colour for pressure-plate gates. Opening and closing reuse the
  gate's movement.
* Dragging keeps its rules from [mirror-drag](mirror-drag.feature.md): flat corridor only, no water, sand or
  subcell stairs, 75% of crouch speed, no attack while dragging.
* Monsters do not block. Living monsters damage the player when next to it, so the player chooses to defend or keep
  dragging.
* Open for the implementer: the exact weight values and the required weight per plate.

## Decisions (2026-11-13, with the user)

* Plates and gates pair by colour: a plate of colour C opens the plate gates of colour C (their own colour set, a
  re-skinned gate), so a level can hold several plate puzzles.
* The rock reuses an existing mesh (no new Blender model).
* Monster bodies are draggable and weigh, accepting that a load loses them (monsters are not saved; plan
  save-live-monsters fixes that later): a plate held by a body may lift after a load.
* No campaign level gets a plate puzzle in this plan: the mechanism, a test level and the checker's model now; a
  follow-up draft places the puzzles.

## Implementation

* `PressurePlate` (27, wall layer, glyph `-`): attribute the lock colour, value the weight it needs. Plate gates are
  `Gate`s of colour `BOSS_LOCK` + the lock colour (6 to 9, glyphs `1` to `4`), silver (the `plate_<colour>` variants
  of the gate in `mechanism.py`); the plate has its own model (`weight_plate`: the dart plate's slab, plain, a gem of
  its colour), so it does not read as a dart trap. `Stone` (28, glyph `;`) is the rock fall's boulder, the same size.
* While every plate of a colour carries its weight, its plate gates go up; under it they come down
  (`GateState::Closing`, the gate's motion backwards, reversing mid-way), not on anyone in the gate's cell. The first
  time a colour's gates go up on a visit: the status line, the field note, the cut-scene to gates off the screen.
* Weights (`WeightOn`): the player `PLAYER_WEIGHT` 2; monsters, alive (not flying, jumping or hidden) or dead,
  `MonsterKind::weight` (a body at least 1: rats and scarabs 1, most 2, giant kin and bosses 3 to 6); a stone 4, an
  emptied chest 2, a mirror 2. The creatures by their centre within 0.35 of the plate's, the things by their cell.
* Dragging: stones, mirrors, emptied chests (any chest kind: treasure, trapped, a mimic's; the moved chest becomes a
  dropped chest, so a save keeps its look) and bodies (they slide with the player). A plate's cell takes a dragged
  thing; any other fixture still refuses it.
* Checker: reaching a plate opens its colour's plate gates (sharing the colour's walker bit with its keys and
  levers); warnings: the things on a plate's flat floor run weigh less than it needs, a plate without plate gates,
  plate gates without a plate.
* Not done: campaign puzzles (a follow-up draft); bodies are lost on a load (plan save-live-monsters).

## Play-test

`bin/game --play tests/play/pressure-plate.txt` (the test corridor):

* Step on the red plate alone: nothing (it needs 3, the player weighs 2). Drag the stone (`R`) onto it: the silver
  gate rises, the plate sinks; walk through.
* Stand on the blue plate: its gate rises; step off: it comes down again, smoothly, back up if you step on mid-way.
* Open and empty the chest, drag it onto a green plate, stand on the other: the green gate rises only with both held.
* Kill the rat and drag its body onto the gold plate: the gold gate rises.
