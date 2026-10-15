# Water decorations

Status: solved 2026-10-14 (play-tested; chance raised to 50%). From the user: plants with no function in the water, so
flooded parts look less bare.

## Idea

Water props: static plants scattered in water cells, like the dry props (`scatterProps`, `src/world/decor_scatter.cpp`).
Today that pass skips every flooded cell (`isEmptyCell` needs `Structure::Empty`), so the water has no props.
No gameplay: no collision, no line-of-sight change, no effect on wading. They are not monsters or objects in the level
file; the level loader scatters them from the cell hash, as it does the other props.

Set (implementer picks the final list, 3 to 5 props):

* **Papyrus reeds**: a clump of tall stems with umbels. Standing in the basin, sticking out of the surface. Prefers half
  water cells next to a wall or dry floor (the bank).
* **Lotus pads**: two or three round leaves flat on the surface, sometimes a flower (blue lotus). Middle of a pool.
* **Water weed**: a short submerged tuft on the basin floor, seen through the surface. Fits the cave tiers.
* **Duckweed / algae**: optional. A green surface decal patch, not a model. Could use the decal pass instead of the props.
* **Deep water weed**: optional. Dark swaying strands in the front of a deep water cell (`deepWaterBox`), so tall
  flooded shafts are not one flat dark front.

Depth tiers, like the dry props (`DECOR_TIERS`, docs/plan/solved/decor-by-depth.md): cave levels get water weed and
reeds, tomb and temple levels add lotus pads (a temple pool).

## Synergies

* [water-lotus](water-lotus.md): that monster looks like a floating flower when idle. With decorative lotus
  pads around it, the ambush works: the player cannot tell it by "a flower = a monster". Make the decor lotus and the
  idle monster look alike. Fairness (user decision): the monster's idle clip has a twitch; the decor lotus never
  twitches. That is the only tell.
* The crocodile and [hippo](../hippo.draft.opus.md) lurk submerged: reeds give the scene a natural place for them, with
  no rule change.

## Rules

* Only in half water cells with no object (`structure == HalfWater`, `type == NoObject`), no slain monster's tile.
  Deep water strands (if done) only in deep water cells whose front is visible.
* Chance per cell: its own constant, like `DECOR_CHANCE_PERCENT` (default 35%; pools should look planted, but not
  full).
* Never in front of a submerged monster's spawn so the plant hides it completely. Default: water props skip monster
  spawn cells.
* Dry props stay out of the water, water props stay out of dry cells.
* Floating props sit at `WATER_SURFACE`, standing ones on the basin floor (`WATER_BASIN_DEPTH` under the row's floor),
  `src/graphics/render_config.h`. Dry props stand on the row's floor; the water ones need their own height.
* Animated preferred (user decision): slow sway (reeds, weed), a bob (pads); a shader wobble or a tiny idle clip,
  implementer's choice. Static is allowed if animation costs too much.
* Draw order: the surface is translucent; parts under it must look under water (tinted by the water), parts above it
  not. Check the reeds where they pass through the surface.

## Sketch

* Extend `DECOR_NAMES` / `DECOR_TIERS` (`src/world/decor.h`) with the water props after the scattered dry ones, or a
  separate water list. Either way `scatterProps` picks from the dry set on dry cells and from the water set on water
  cells.
* `DECOR_JITTER` entries for the new props, from the extents `decor.py` prints.
* The scenario `prop` command (`src/test/scenario_script.cpp`) then works for water props too.

## Model

Procedural, in `tools/blender/models/decor.py` like the other props ([docs/remodeling.md](../../remodeling.md)), tile
units, origin at the floor centre. Low poly, alpha-tested leaves where it saves polys.

## Tests

* Unit: water props only on half water cells, dry props never there; none on spawn cells; same seed, same layout.
* Scenario with a screenshot: a pool on a flooded level with reeds at the bank, pads in the middle; one under water
  view (player wading) to check the surface draw order.
* Glossary row: water prop.

## Implemented (2026-10-14)

* Four props after the coffin in `DECOR_NAMES` (`src/world/decor.h`): `water_reeds`, `water_weed`, `water_pads`,
  `water_lotus` (`isWaterProp`). Models: `tools/blender/models/decor.py`, at their real height in the basin.
  Duckweed and the deep water strands (optional) are not done.
* Scatter: `scatterWaterProp` (`src/world/decor_scatter.cpp`), 50% (`WATER_DECOR_CHANCE_PERCENT`, first 35%, raised after the play-test) of half water cells
  with nothing in them (no monster spawn, item, fixture or slain monster). Bank cells (a row neighbour not half water)
  favour the reeds, the middle of a pool the leaves (`WATER_BANK_WEIGHT`, `WATER_MIDDLE_WEIGHT`). Dry layouts are
  unchanged (the golden dumps only gained water props).
* Tiers: reeds and weed from the caves, the leaves and the lotus from the worked tunnels (tier 1, level 4), not the
  tombs: the [water-lotus](water-lotus.md) monster needs decor lotus to hide among from its first levels.
* Motion at draw time (`waterMotion`, `src/world/dungeon_render_decor.cpp`): reeds and weed sway (x sheared by the
  height over the basin floor), the leaves bob and drift; each cell on its own phase. No clips.
* Draw order: nothing new. The water is drawn after the props, see-through, so the parts under the surface are tinted
  and the parts above are not. In the toon view the weed under the water gets ink lines, as the player's legs do.
* Tests: `tests/unit/decor_test.cpp` (water props only in free half water, dry props never there, share about 50%, no
  lotus in the caves), golden dumps, scenario `tests/scenarios/solved/water_decorations.txt` (screenshots).

Play-test: `bin/game --level 21` (24 water props), or `--level 16` / `--level 25`.
