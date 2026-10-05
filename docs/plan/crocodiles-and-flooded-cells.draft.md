# Crocodiles and flooded cells

Status: draft 2026-10-05. Needs [level-format-layers.draft.md](level-format-layers.draft.md) first.

## Water cells

Two kinds:

* **Half water**: half filled. The player walks in it at **50%** speed (walk and sprint).
* **Deep water**: full of water, decoration only. It goes below half water and is the floor under it: nobody enters
  it, it is solid like a wall for movement.

Rules:

* No drowning.
* No jumping while in half water (open: or only no jumping into it?).
* A ladder can start in half water and go up. No ladder goes down into water.

Look: a water surface at half the cell height, a slow ripple, the torch light reflected on it. The player and the
monsters are hidden up to the waist. Deep water is darker, no surface of its own. Sound: splashing steps.

Open:

* Arrows, rock falls and traps in water.
* Falling into half water from a height: lands like on a floor?

## Monsters in water

A new per-type field in `MONSTER_DEFS`, how the monster moves in half water (for example `enum class Wading`):

| Wading | Speed in half water | Monsters |
|---|---|---|
| Slowed | 50%, like the player | scarab, giant scarab, boss scarab, worm, mummy (and the scorpions) |
| Unaffected | 100% | rat, giant rat, Anubis, Anubis boss |
| Swimmer | 125% | crocodile, cobra, Apep ([apep-serpent-boss.draft.md](apep-serpent-boss.draft.md)) |
| Not walking | - | plant, mimic (rooted), bat, giant bat, vampire bat (fly) |

Rooted monsters and flyers need no value: their `Locomotion` already says it. So the field may only need the first
three.

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

A cell has one type (`Tile` in `src/world/level.h`: type, attr, value). Deep water is easy: a new tile type, solid like
`Wall`. Half water has to sit on top of other cells: an empty cell, a monster spawn (the crocodile's own spawn), the foot
of a ladder, maybe a key or a treasure. So a new tile type alone is not enough for half water.

Decided (2026-10-05): a structure layer under the objects, see
[level-format-layers.draft.md](level-format-layers.draft.md). `HalfWater` and `DeepWater` are structure types, so a
spawn, a ladder or a key can stand in half water. That plan comes first.

Speed: the player and the monsters read whether the cell they stand in is half water, and scale their speed from
their wading kind ([Monsters in water](#monsters-in-water)); the player wades like Slowed.

## Checks

* `levelcheck`: half water stands on deep water or a wall; deep water only under half water or other deep water; no
  ladder goes down into water.
* `levelcheck`: crocodiles only spawn in or next to water; the way to the exit must not need a long swim past a
  crocodile without dry ground nearby (open).
* Scenario test: player speed in and out of water, crocodile speed in and out of water.
