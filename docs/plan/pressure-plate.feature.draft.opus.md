# Pressure plates open gates by weight

Status: draft 2026-11-13, size L, needs [mirror-drag](solved/mirror-drag.feature.md) first.

## Idea

A new puzzle: a pressure plate opens a gate while enough weight stands on it. Step off, and the gate closes. The
solution is to drag a heavy object onto the plate. Dragging comes from [mirror-drag](solved/mirror-drag.feature.md).

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
* Dragging keeps its rules from [mirror-drag](solved/mirror-drag.feature.md): flat corridor only, no water, sand or
  subcell stairs, 75% of crouch speed, no attack while dragging.
* Monsters do not block. Living monsters damage the player when next to it, so the player chooses to defend or keep
  dragging.
* Open for the implementer: the exact weight values and the required weight per plate.

## Play-test

*
