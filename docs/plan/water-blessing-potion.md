# Potion of water blessing (Blessing of Hapi)

Status: implemented 2026-10-17, waiting for play-test: `bin/game --play tests/play/water-blessing-potion.txt` (potion
alone), `bin/game --play tests/play/water-blessing-potion-skill.txt` (with the Favour of Hapi skill). Draft 2026-10-09,
refined 2026-10-09. From the user.

It also adds the stacking part of [skill-blessing-of-hapi](solved/skill-blessing-of-hapi.md): with the Favour of Hapi skill the
potion lasts 4 minutes, the wade speed is 125% and the sprint in half water is the full land sprint with half the
stamina drain (the skill part was in already; its table has the numbers).

## Idea

A potion that lifts the water's limits from the player for 2 minutes. Only the player: monsters, arrows and the
crocodile's swim are unchanged.

## What water does to the player today

From [crocodiles-and-flooded-cells](solved/crocodiles-and-flooded-cells.md#implemented):

| Limit | Where | With the blessing |
|---|---|---|
| Walk and climb at 50% (`WADE_SPEED_FACTOR`) | `input.cpp` | full speed |
| No sprint while wading (no speed, no stamina drain) | `input.cpp` / sprint | sprint at 40% of the land sprint speed, stamina drains as on land |
| No jump while wading ("too deep to jump") | `Dungeon::JumpAllowed` | jump as on land |

Not changed: wading still splashes (`WADE_SPLASH_MS`, `Dungeon::PlayerWading`), the player still sinks into the
basin (`waterSink`), arrows into water still lose damage (a monster rule).

## Sketch

* A new potion `ItemKind` after `GreaterResistance`. It goes at the end of the potions, so the amulets shift again and
  the bag save needs a new version, as `INV5` did for the resistance potions.
* `PlayerStats`: time left (like the resistance potion's `RESIST` save line), ends on death. The checks above ask
  e.g. `PlayerStats::WaterBlessed()` / `Dungeon::PlayerWading` callers skip their limit.
* Drinking another restarts the 2 minutes (4 with the Favour of Hapi skill,
  [skill-blessing-of-hapi](solved/skill-blessing-of-hapi.md)).
* HUD: a timer icon like the resistance potion's, if it has one; the status box line on the start and the end.
* Model / texture: a potion vessel (`PotionModel`) with its own texture, a blue-green liquid; journal note.
* Loot: see [Decided](#decided-2026-10-09).
* `levelcheck`: models "no jump out of half water". The potion is optional, so the checker keeps assuming none and
  levels stay passable without it.
* Tests: unit (timer, death, save), a scenario in `tests/scenarios/` (wade speed, sprint and jump out of half water with
  and without it, it wears off).

## Decided (2026-10-09)

* **Name:** "Blessing of Hapi" (the Nile god), short name "Hapi". Fits the Egyptian names (the antidote's
  Renenutet). The journal text is open for the implementer.
* **Grade:** one, 2 minutes. No lesser / greater pair.
* **Effects:** only the three limits in the table (wade speed, sprint, jump in half water). The crocodile's bite stays
  as it is; deep water stays blocked.
* **Drops:** hand-placed in campaign chests on levels with half water, like the antidote on levels with poisoners.
  `generatedWeight = 0`, `mimicLoot = false`.

## Open (for the implementer)

* How many per level: about one per water level; pick the levels and chests, and keep `levelcheck` clean.

## Implemented

* `ItemKind::WaterBlessing` (potion file id 10, after `GreaterResistance`), `PotionGain::waterBlessing`,
  `PotionEffect::WATER_BLESSING_MS` (120000, twice with the skill). Bag saves are `INV7`; `INV5` / `INV6` / older load
  with the amulet slots shifted (`SlotShift` in `item_bag.cpp`).
* `PlayerStats::WaterBlessed`, `WaterBlessingLeftMs`, `AdvanceWaterBlessing` (status "The water is deep again"),
  saved as a `WATER <ms>` line after `RESIST`; ended on death and by `Reanimate`.
* `Dungeon::PlayerWalkFactor`, `PlayerSprintMultiplier`, `JumpAllowed`, `PlayerHalvedWadeDrain` (input.cpp
  `NoteWalked`, mirrored in `tests/unit/sim_world.h`); constants `WADE_*_BLESSED` / `*_BOTH` in
  `src/core/gameplay_config.h`. Halved drain: `PlayerStats::NoteWalked(true)`.

| | Wade | Sprint in half water | Stamina drain | Jump |
|---|---|---|---|---|
| Potion | 100% | 40% of the land sprint | as on land | yes |
| Potion + skill | 125% | full land sprint | half | yes |

* HUD: a blue-green drop tile with the seconds left, right of the resistance tile. Journal note "Blessing of Hapi"
  (`FieldNote::WaterBlessing`, written on the first pickup).
* Vessel `PotionModel::Pilgrim` (the water flask, shared with the might potion) with its own baked texture
  `media/textures/items/potion_water_blessing.png` (`POTIONS` in `tools/blender/models/items.py`, baked with Blender 5
  `-- --export --only potion_pilgrim`; only the new texture was kept).
* Loot: one chest with the potion (`x y 8 3 10` in the level file) on each water level, three cells before the first
  half water on its row: levels 7, 8, 9, 16-25, 27-30. Level 1 has one lone half-water cell, no potion there. The
  checker keeps assuming none; the levels have no warnings. A chest cell lost its decor prop in the decor goldens of
  levels 8, 9, 24, 25, 28, 29.
* Tests: `tests/unit/water_blessing_test.cpp` (timer, restart, end line, death, save and load, speed / sprint / drain /
  jump table, 4 minutes with the skill), `items_test.cpp` (file id, `INV7`, `INV6` and `INV4` loads),
  `tests/scenarios/water_blessing_potion.txt`.
