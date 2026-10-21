# Campaign zones: 40-60 levels in distinct zones

Status: draft 2026-10-21, from the user. Needs nothing first. Open: the zone list, the level count (40-60), and where
the two new bosses go. Propose these to the user before writing code.

## Idea

The final campaign has 40-60 levels, grouped into zones of several levels each. A zone has its own place in the
story, look, monster set and main mechanic, and most zones end with a boss. Today only the desert has a range in code
(`DESERT_FIRST`..`DESERT_LAST`, `src/world/campaign.h`); everything else depends on the level number.

This plan fixes the shape of the campaign. The other plans build on it: [monster-balance](monster-balance.draft.opus.md)
balances against its monster sets and level count, [campaign-levelgen](campaign-levelgen.draft.opus.md) builds its
levels, [zone-looks](zone-looks.draft.opus.md) gives each zone its look,
[campaign-texts](campaign-texts.draft.sonet.md) writes its texts, and [denser-levels](denser-levels.draft.opus.md)
rebuilds the campaign zone by zone.

## Zones

Today's chapters already read like zones: 1-5 the tomb entrance (Khepri), 6-10 the bat roosts (Bat of Shezmu),
11-14 the mummy halls, 15 the scorpion nest (scorpion queen), 16-20 the serpent temple (Apep), 21-25 the desert
(Seth), 26-30 the cisterns (Sobek), 31-35 the necropolis (the High Priest on 34, Anubis on 35).

Bosses: 8 today, 10 with [camel-spider-boss](camel-spider-boss.draft.opus.md) and [worm-boss](worm-boss.draft.opus.md).
About 10-12 zones of 4-5 levels gives 40-60 levels. Suggestions, the user's choice:

* The scorpion queen gets a zone of her own: the mummy halls stop being a gap before her.
* The High Priest gets a zone of his own (the priests' quarter), separate from Anubis's necropolis.
* The spider's zone: dark halls full of webs (`DECOR_WEB`), camel spiders.
* The worm's zone: a second desert zone, the land of Sokar (the Amduat's fourth and fifth hours, sand). Seth keeps the
  oasis.
* Zones without a boss are allowed (user: "others remain without a boss"), for example a short zone between two
  long ones.
* [duat-hours](duat-hours.draft.sonet.md) (on hold) names the twelve hours of the Duat. Twelve zones could be the
  twelve hours: each zone is one hour behind a gate, with the gate's guardian as its boss. Then duat-hours becomes
  the zones' names and banners.

## What changes

* A zone table in code (`src/world/campaign.h` or its own file): id, name, first and last level, stone, monster set,
  boss, main mechanic (water, sand, dark, ...). `zoneOf(level)` replaces `desertLevel` and the other level-number
  checks (grep `DESERT_FIRST`, `LEVEL_` ranges, `levelNumber >=`).
* `CAMPAIGN_LEVELS` comes from the table. Until [denser-levels](denser-levels.draft.opus.md) rebuilds the campaign,
  the table maps today's 35 levels. The target table (40-60 levels) goes in `docs/levels.md` as a design table and
  becomes the code table when the rebuild switches over.
* `docs/levels.md`: a section per zone (levels, story, monsters, mechanic, boss) above the per-level table.
* Glossary: a `zone` row.
* The boss drafts' open placement: a zone slot each. Update [camel-spider-boss](camel-spider-boss.draft.opus.md) and
  [worm-boss](worm-boss.draft.opus.md) if they are still drafts. If they are already done, they move with the rebuild.

## Tests

* Unit: the zones cover 1..`CAMPAIGN_LEVELS` without gaps or overlaps, each boss is in exactly one zone, and the last
  zone ends with the ankh level.
* Today's campaign plays as before (the table maps it 1:1).
