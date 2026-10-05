# Crocodiles and flooded cells

Status: draft 2026-10-05, open points decided 2026-10-05. Builds on the level format v2 ([solved/level-format-layers.md](solved/level-format-layers.md),
done).

## Water cells

Two kinds:

* **Half water**: half filled. The player walks in it at **50%** speed (walk and sprint).
* **Deep water**: full of water, decoration only. It goes below half water and is the floor under it: nobody enters
  it, it is solid like a wall for movement.

Rules:

* No drowning.
* No jumping while standing in half water. Jumping into it is fine, so water acts as a trap: the player gets in but
  cannot jump out (decided 2026-10-05). `levelcheck` models this.
* Falling into half water from a height lands like on a floor: no extra damage, a splash sound.
* A ladder can start in half water and go up. No ladder goes down into water.
* Arrows: a target standing in half water takes reduced arrow damage (number open, for example 50%). Melee weapons
  are not affected.
* Traps (spikes, death traps, rock falls) work in half water as on dry ground and stay visible: the water is
  semi-transparent.

Look: a semi-transparent water surface at half the cell height, a slow ripple, the torch light reflected on it. The
player and the monsters show dimmed up to the waist. Deep water is darker, no surface of its own. Sound: splashing
steps.

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
* Amphibious: it leaves the water freely and walks on dry ground at its normal speed, without the water bonus.
* Idle, it lies under the surface with only the eyes and the back showing, like the mimic's ambush.
* A plain bite, no hold. A bite with a hold is a separate idea:
  [crocodile-hold-bite.draft.md](crocodile-hold-bite.draft.md).
* Strength: between the giant rat and the mummy, with higher damage than both (numbers when the work starts, against
  [monster-balance.draft.md](monster-balance.draft.md)). First crocodiles around lvl 7-9; those levels get water.
* Model built in Blender ([../remodeling.md](../remodeling.md)). A Sobek boss could follow later
  ([more-bosses.draft.md](more-bosses.draft.md)).

Gameplay: the player is at half speed and the crocodile 25% faster, so fleeing through water does not work. Fight on
the bank or with the bow from dry ground.

## How to store water

A cell has one type (`Tile` in `src/world/level.h`: type, attr, value). Deep water is easy: a new tile type, solid like
`Wall`. Half water has to sit on top of other cells: an empty cell, a monster spawn (the crocodile's own spawn), the foot
of a ladder, maybe a key or a treasure. So a new tile type alone is not enough for half water.

Decided (2026-10-05): a structure layer under the objects, see
[solved/level-format-layers.md](solved/level-format-layers.md). `HalfWater` and `DeepWater` are structure types, so a
spawn, a ladder or a key can stand in half water. Done: the format, the editor and the checker know water; it has
no game rules yet.

Speed: the player and the monsters read whether the cell they stand in is half water, and scale their speed from
their wading kind ([Monsters in water](#monsters-in-water)); the player wades like Slowed.

## Checks

* `levelcheck`: half water stands on deep water or a wall; deep water only under half water or other deep water; no
  ladder goes down into water.
* `levelcheck`: crocodiles only spawn in or next to water.
* `levelcheck` treats a crocodile as a wall the path cannot swim past: separate draft,
  [levelcheck-crocodile-wall.draft.md](levelcheck-crocodile-wall.draft.md).
* Scenario test: player speed in and out of water, crocodile speed in and out of water.
