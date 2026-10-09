# Potion of water blessing

Status: draft 2026-10-09 (idea, not decided). From the user.

## Idea

A potion that lifts the water's limits from the player for 2 minutes. Only the player: monsters, arrows and the
crocodile's swim are unchanged.

## What water does to the player today

From [crocodiles-and-flooded-cells](solved/crocodiles-and-flooded-cells.md#implemented):

| Limit | Where | With the blessing |
|---|---|---|
| Walk and climb at 50% (`WADE_SPEED_FACTOR`) | `input.cpp` | full speed |
| No sprint while wading (no speed, no stamina drain) | `input.cpp` / sprint | sprint as on land, stamina drains as on land |
| No jump while wading ("too deep to jump") | `Dungeon::JumpAllowed` | jump as on land |

Not changed: wading still splashes (`WADE_SPLASH_MS`, `Dungeon::PlayerWading`), the player still sinks into the
basin (`waterSink`), arrows into water still lose damage (a monster rule).

## Sketch

* A new potion `ItemKind` after `GreaterResistance`. It goes at the end of the potions, so the amulets shift again and
  the bag save needs a new version, as `INV5` did for the resistance potions.
* `PlayerStats`: time left (like the resistance potion's `RESIST` save line), ends on death. The checks above ask
  e.g. `PlayerStats::WaterBlessed()` / `Dungeon::PlayerWading` callers skip their limit.
* Drinking another restarts the 2 minutes.
* HUD: a timer icon like the resistance potion's, if it has one; the status box line on the start and the end.
* Model / texture: a potion vessel (`PotionModel`) with its own texture, a blue-green liquid; journal note.
* Loot: chests on levels with water only, like the antidote on levels with poisoners (`mimicLoot`,
  `generatedWeight`).
* `levelcheck`: models "no jump out of half water". The potion is optional, so the checker keeps assuming none and
  levels stay passable without it.
* Tests: unit (timer, death, save), a scenario in `tests/scenarios/` (wade speed, sprint and jump out of half water with
  and without it, it wears off).

## Open

* Name: "Potion of water blessing", or an Egyptian one (Hapi, the Nile god; Sobek, the crocodile god).
* 2 minutes, one grade only, or a lesser / greater pair like the resistance potions?
* Does it also make the crocodile's bite weaker, or let the player swim through deep water? (Today: only the limits
  above.)
* Where does it drop, and how often?
