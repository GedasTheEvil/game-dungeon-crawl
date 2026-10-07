# Walls, decals and decorations by depth

Status: draft 2026-10-07, from the user.

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
| Tomb | 8 | ushabti, cat, Bes | plaster, worn plaster, slab floor, slab ceiling | glyph rows |
| Temple / necropolis | 13 | jackal (Anubis), Osiris, sarcophagus | broken plaster, star ceiling, painted walls | full painted scenes |

* The mummy's coffin (`DECOR_COFFIN`) stays where it is: it belongs to the mummy, not to the tier.
* Today the share of rough walls grows with depth (`ROUGH_PERCENT_FIRST`, `ROUGH_PERCENT_STEP`): that runs the wrong
  way and gets reversed. Lots of rough cave early, less and less deeper down.
* Within a tier, newer things could be more common than older ones, so each depth gets its own look, while a few
  cave bits still turn up deep down.
* The tier table goes in one place, a data table like `MONSTER_DEFS`, so the level ranges are easy to tune.

## Open

* The level ranges per tier, and which prop goes in which tier.
* Decals: the current glyph set may need simpler ones for the early tiers (or none).
* Generated levels (`levelgen`) and test levels have no campaign number: pick a tier from the generator's difficulty,
  or use the deepest tier.
