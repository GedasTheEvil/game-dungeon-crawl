# Walls, decals and decorations by depth

Status: solved 2026-10-08, play-tested. Implemented 2026-10-07. Draft 2026-10-07, from the user.

Today every level draws from the same pool. The props (`DECOR_NAMES`, `src/world/decor.h`), the wall decals
(`tools/textures/decals.py`) and the wall, floor and ceiling surfaces (`textures/dungeon/`) are all picked at random
in `dungeon_decor.cpp`. So the first level can already show an Anubis statue, a sarcophagus or painted plaster.

## Idea

* Group the decorations into tiers by depth. A level picks at random only from the tiers unlocked so far.
* The tiers add up: a deeper level keeps every earlier tier and adds new things. It does not swap one set for
  another.
* The first levels look like a natural cave: rough rock walls, sand floors, no worked stone, no statues, no mummy
  props. The deeper levels look more and more like a built and painted tomb.

## Proposed tiers (to tune)

| Tier | From level | Props added | Surfaces added | Decals |
|---|---|---|---|---|
| Cave | 1 | web, rubble, sand, skeleton, pottery | rough walls, strata, sand floor, rough ceiling | none, or scratches |
| Worked tunnel | 4 | canopic, lamp, brazier, scrolls | dressed stone, cracked stone, cracked floor | a few simple glyphs |
| Tomb | 8 | ushabti, cat, Thoth (four variants) | plaster, worn plaster, slab floor, slab ceiling | glyph rows |
| Temple / necropolis | 13 | jackal (Anubis), Osiris, sarcophagus | broken plaster, star ceiling, painted walls | full painted scenes |

* The mummy's coffin (`DECOR_COFFIN`) stays where it is: it belongs to the mummy, not to the tier.
* Today the share of rough walls grows with depth (`ROUGH_PERCENT_FIRST`, `ROUGH_PERCENT_STEP`): that runs the wrong
  way and gets reversed. Lots of rough cave early, less and less deeper down.
* Within a tier, newer things could be more common than older ones, so each depth gets its own look, while a few
  cave bits still turn up deep down.
* The tier table goes in one place, a data table like `MONSTER_DEFS`, so the level ranges are easy to tune.

## Decided (2026-10-07, the user let the agent pick; easy to change later)

* **Tiers and props:** as in the table above. Cave from level 1, worked tunnel from 4, tomb from 8, temple /
  necropolis from 13. The tiers stop at 13: levels 13-30 get the whole set, weighted towards the temple tier.
* **Weights:** the newest tier unlocked counts double, so each depth gets its own look and older things still turn
  up.
* **Decals:** none in the cave tier (bare rock), only the simple single glyphs in the worked tunnel (ankh, reed,
  water, eye), all glyphs from the tomb tier on, and the painted bands and scenes only in the temple tier.
* **Generated levels:** `levelgen`'s difficulty 1-10 maps to a level, 1 + 1.5 x (difficulty - 1), which picks its
  tier. Test levels without a campaign number use the deepest tier, so scenarios keep seeing every prop.

## Done (2026-10-07)

* The tier table in one place, `src/world/decor.h`: `DECOR_TIER_FROM` (1, 4, 8, 13), `DECOR_TIERS` per prop,
  `DECAL_TIERS` per decal, `ROUGH_PERCENT` / `PAINTED_PERCENT` per tier, and the tiers that unlock the cracked floor,
  the slab floor and the starry ceiling. `decorTier(depth)`.
* `Dungeon::scatterDecorations(levelName, depth)`: props and decals pick by weight among the unlocked tiers, the newest
  one double (`tierWeight`, `weightedPick` in `dungeon_decor.cpp`). Thoth's four variants count as one pick.
* Surfaces: all rough rock and sand in the cave tier (levels 1-3); dressed stone and cracked floors from 4; plaster,
  slab floors and ceilings from 8; the starry ceiling over painted stretches from 13. The old "more rough rock deeper
  down" (`ROUGH_PERCENT_FIRST` / `_STEP`) is reversed: 100%, 55%, 30%, 15% rough rows.
* Depth: the campaign level (also for a loaded save); `genDecorDepth(difficulty)` in `level_gen.h` for a generated
  level (1 + 1.5 x (difficulty - 1), rounded down); any other level (test levels) `DECOR_DEPTH_ALL`, every tier.
* Tests: `tests/unit/decor_test.cpp`, `tests/scenarios/decor_depth.txt` (new `expect decor_tier`: the highest tier in
  the level; lvl1 is all cave, lvl20 temple). `statues.txt` now places its props with the `prop` command, since the
  hash no longer puts them where it did.

Decided on the way (easy to change):

* **Natural decals in every tier.** Cracks, vines, roots, seepage, moss, dry grass, creepers and the papyrus plant are
  cave things too, so the cave keeps them; only the man-made ones wait for their tier. "None in the cave tier" read as
  no glyphs.
* **Glyphs:** the worked tunnels get the single eye of Horus (the ankh / reed / water glyphs named above are not
  decals), the tombs the glyph columns, rows and cartouches, the temples the painted winged sun.
* **Broken plaster from the tomb tier**, not the temple tier: it is the seam between a painted stretch and dressed
  stone, needed wherever plaster is.
* **Slab ceilings** come with the dressed stone (worked tunnel), not only with the tomb tier: the stone needs a ceiling.
