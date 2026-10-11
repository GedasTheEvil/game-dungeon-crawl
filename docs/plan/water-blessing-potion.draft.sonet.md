# Potion of water blessing (Blessing of Hapi)

Status: draft 2026-10-09, refined 2026-10-09 (decided, not implemented). From the user.

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
