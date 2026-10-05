# Crocodiles and flooded cells

Status: draft 2026-10-05. Implementation still unclear, see [How to store water](#how-to-store-water).

## Flooded cells

* A cell can be **flooded**: half filled with water.
* The player moves at **50%** speed in a flooded cell (walk and sprint).
* Look: a water surface at half the cell height, a slow ripple, the torch light reflected on it. The player and the
  monsters are hidden up to the waist.
* Sound: splashing steps.

Open:

* Jumps: lower, or the same height? Ladders standing in water?
* Is flooding a cell ever dangerous by itself (drowning)? Proposal: no, only slow.
* Do other monsters slow down in water too (rats, scarabs, mummies), or avoid it like a trap (`Courage`)?
  Flyers (bats) do not care.
* Arrows, rock falls and traps in water.

## Crocodile

* New monster, at home in the water: in a flooded cell it moves **125%** of its normal speed.
* It can leave the water and keeps its normal speed on dry ground. Or it stays in the water (proposal: it may leave,
  but goes back after losing the player).
* Idle, it lies under the surface with only the eyes and the back showing, like the mimic's ambush.
* A bite with a hold? Open.
* Model built in Blender ([../remodeling.md](../remodeling.md)). A Sobek boss could follow later
  ([more-bosses.draft.md](more-bosses.draft.md)).

Gameplay: the player is at half speed and the crocodile 25% faster, so fleeing through water does not work. Fight on
the bank or with the bow from dry ground.

## How to store water

A cell has one type (`Tile` in `src/world/level.h`: type, attr, value). Water has to sit on top of other cells: an empty
cell, a monster spawn (the crocodile's own spawn), maybe a key or a treasure. So a new `Water` tile type is not
enough on its own.

Options:

1. **A flood layer in `Level`**: one flag per cell next to `cells`. Saved in the level file, drawn in the editor as
   an overlay. Cleanest, but changes the level format (see
   [binary-level-format.draft.md](binary-level-format.draft.md)), the editor, `ascii2level.py` and `levelcheck`.
2. **A flag bit in the tile**: for example a high bit in `attr` or `value`. Fits the current format, but those fields
   already mean different things per type.
3. **A `Water` tile type** that only stands alone, with monster spawns allowed next to it (a crocodile spawns on the
   bank and walks in). Simplest, the fewest places to touch, but no keys or treasure under water.

Proposal: start with 3 to try the feel, move to 1 if water must hold other things.

Speed: the player and the monsters read a speed factor of the cell they stand in (water 0.5 for the player, 1.25
for the crocodile, 1 otherwise). A per-monster-type factor in `MONSTER_DEFS` (or a new `Locomotion::Swim`) keeps it
data-driven.

## Checks

* `levelcheck`: crocodiles only spawn in or next to water; the way to the exit must not need a long swim past a
  crocodile without dry ground nearby (open).
* Scenario test: player speed in and out of water, crocodile speed in and out of water.
