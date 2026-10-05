# Crocodiles and flooded cells

Status: draft 2026-10-05, open points decided 2026-10-05. Implemented 2026-10-05, see [Implemented](#implemented);
waits for a check in play. Builds on the level format v2 ([solved/level-format-layers.md](solved/level-format-layers.md),
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

Gameplay: the player is at half speed and the crocodile 25% faster. Fight on the bank or with the bow from dry ground.
Open (found in the implementation): monsters are much slower than the player in this game (`MONSTER_SEEK_STEP`), so at
the giant rat's speed (4) the crocodile swims 0.3 tiles/s and the wading player still outruns it (0.5 tiles/s).
Fleeing through water only fails from a land speed of about 7 (or a bigger swim bonus).

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

## Implemented

* Rules (`src/core/gameplay_config.h`): `WADE_SPEED_FACTOR` 0.5 (the player's walk and climb in `input.cpp`, slowed
  walkers in `Monster::Seek`), `SWIM_SPEED_FACTOR` 1.25, `ARROW_WATER_DAMAGE_PCT` 50 (an arrow into a monster whose centre
  is in half water). No jump while wading (`Dungeon::JumpAllowed`, with a "too deep to jump" line); a fall or a jump into
  water lands as on a floor, with a splash. Splashing steps every 450 ms while wading. `Dungeon::PlayerWading`.
* Wading kinds: `Wading` in `src/entities/monster.h`, set per type in `WADING_DEFS` (`src/state/assets.cpp`): rats and
  both Anubis unaffected, the crocodile a swimmer, everyone else slowed.
* Look: `Dungeon::DrawWater`, after the player and the monsters, see-through without depth writes: half water a teal
  front up to half the cell, a surface with drifting glints and a bright waterline; deep water a dark front over the
  whole cell. Traps, the player and the monsters show dimmed under it. No floor decals under water.
* Crocodile: `MonsterCrocodile` (15), glyph `C`, `Locomotion::Submerged`: lies still (idle clip) with only its top
  `SUBMERGED_SHOW` above the water, wakes when the player comes within `SUBMERGED_WAKE_RANGE` (1.4 tiles from its
  centre: the camera shows about 1.5 tiles ahead, so the lurk is seen first) or hits it. A swimmer floats with its back at
  the surface while its head is over water, and walks on the floor once its head is over dry ground. Speed 4, 110 HP,
  26 damage every 1100 ms, 2400 XP, the scutes resist the sword (slash). Journal: "Lies under the water, only its eyes
  show." Model, texture and sounds: `tools/blender/models/crocodile.py`, `tools/audio/crocodile_sounds.py`
  ([../remodeling.md](../remodeling.md)); water sounds `tools/audio/water_sounds.py`.
* `levelcheck`: no jump out of half water; warnings for half water not on deep water or a wall, deep water not under
  water, a ladder down into water, a crocodile not in or next to water. Unit tests `tests/unit/water_test.cpp`.
* Levels: lvl7 (a side pool in the west hall, the way to a chest), lvl8 (the hall to the red gate), lvl9 (the lower
  gallery past the pit), one crocodile each. All campaign levels pass `levelcheck` with no warnings, `make paths` passes.
* Scenarios: `tests/scenarios/water.txt` (wade speed, no jump, spikes in water, a ladder out of it),
  `tests/scenarios/crocodile.txt` (lurk, wake, swim, bite at the bank, half and full arrow damage, the kill).
* Not done: a splash when a monster enters the water; the crocodile's sounds are synthesized, check them in play.
