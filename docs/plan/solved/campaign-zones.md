# Campaign zones: 40-60 levels in distinct zones

Status: draft 2026-10-21, from the user. Needs nothing first. Zone list decided 2026-10-21 (user: "I like it"), see
[Zones](#zones-decided-2026-10-21); the Lake of Fire added the same day as the twelfth zone (user).

## Idea

The final campaign has 40-60 levels, grouped into zones of several levels each. A zone has its own place in the
story, look, monster set and main mechanic, and most zones end with a boss. Today only the desert has a range in code
(`DESERT_FIRST`..`DESERT_LAST`, `src/world/campaign.h`); everything else depends on the level number.

This plan fixes the shape of the campaign. The other plans build on it: [monster-balance](../monster-balance.draft.opus.md)
balances against its monster sets and level count, [campaign-levelgen](../campaign-levelgen.draft.opus.md) builds its
levels, [zone-looks](../zone-looks.md) gives each zone its look,
[campaign-texts](../campaign-texts.draft.sonet.md) writes its texts, and [denser-levels](../denser-levels.draft.opus.md)
rebuilds the campaign zone by zone.

## Zones (decided 2026-10-21)

Twelve zones, about 54 levels, four acts: down from the tomb, through the temple, out to the river and the sands, into the
Duat. Each zone brings one new main mechanic; its boss is the kin of the zone's monsters (AGENTS.md, Boss
abilities). Level ranges are about, not exact.

| # | Zone | Levels | Monsters | New mechanic | Boss |
|---|---|---|---|---|---|
| **Act I: the tomb** (limestone) |||||
| 1 | Tomb entrance | 1-4 | rats, scarabs, bats, plants | spikes, pits, the first key and riddle | Khepri |
| 2 | Bat galleries | 5-9 | bats, giant bats, worms, giant rats | levers, rock falls, a short first wade (one crocodile) | Bat of Shezmu |
| 3 | Web halls | 10-13 | camel spiders, giant scarabs, mimics | darkness and light (spiders shun torches), trapped chests | spider boss ([camel-spider-boss](../camel-spider-boss.draft.opus.md)) |
| **Act II: the temple** (painted plaster) |||||
| 4 | Priests' quarter | 14-18 | mummies, living ushabti, the first Anubis guards | name gates, many inscriptions, fire jets (braziers), ring transporter | High Priest |
| 5 | Scorpion nest | 19-22 | scorpions, giant scorpions | poison drips, antidotes | scorpion queen |
| 6 | Serpent temple | 23-27 | cobras, giant cobras | dart plates, rhythm traps | Apep |
| **Act III: the river and the sands** |||||
| 7 | Cisterns of Sobek (wet stone) | 28-32 | crocodiles, hippos, giant cobras | deep water, swimming, Hapi potions | Sobek |
| 8 | Seth's desert (sandstone) | 33-37 | camel spiders, giant scorpions, hippos at the oasis | sand, mirrors and sun discs | Seth |
| 9 | Land of Sokar (sandstone) | 38-41 | sand worms, camel spiders in the dark | quicksand, sand falls, deep drifts | worm boss ([worm-boss](../worm-boss.draft.opus.md)) |
| **Act IV: the Duat** (dark granite) |||||
| 10 | Lake of Fire (black basalt, red light) | 42-45 | baboons ([baboon](../baboon.draft.opus.md)), fire-spitting cobras, giant bats | fire everywhere: fire-jet runs, burning floors, braziers; water as the refuge ([fire-in-water](../fire-in-water.draft.sonet.md)) | Babi ([babi-boss](../babi-boss.draft.opus.md)) |
| 11 | Necropolis | 46-50 | Anubis guards, mummies in coffins, cursed mimics, a few baboons | every trap mixed, the Stargate chamber, the last fork | none |
| 12 | Hall of Judgment | 51-54 | Anubis guards, the best of every zone | riddle-heavy finale, the weighing of the heart | Anubis |

Why:

* Boss strength follows the order. Fixes today's Sobek (1600 HP, lvl30) after Seth (2000 HP, lvl25).
* The spider early, near the camel spiders' first levels; darkness early teaches the light the mirrors use later.
* The worm gets the second desert zone (the Amduat's land of Sokar), so Seth keeps the oasis.
* The High Priest moves from lvl34 to about lvl18 and ties to the priests' writing (inscriptions, name gates). The
  necropolis is the one zone without a boss (user: "others remain without a boss"): a build-up before the finale.
* Forks ([denser-levels](../denser-levels.draft.opus.md#route-fork-left-or-right)): a small one in the bat galleries or
  the web halls, one in the priests' quarter, the big one in the necropolis.

Costs:

* Crocodiles move from 7-9 to 28 on; only the one preview wade in zone 2 stays early.
* The High Priest needs retuning for about lvl18 ([monster-balance](../monster-balance.draft.opus.md)).

* The Lake of Fire (Book of the Dead 126, the Amduat): the gate of the Duat, guarded by four baboons; Babi eats the
  hearts of the unworthy there. The step from the sands into the night. New: the baboon, a ranged kiter
  ([monster-kiting](../monster-kiting.md)), and its boss Babi. Fire jets come in with the priests' quarter
  and return in force here.

## The twelve hours of the Duat

(Merged in from the duat-hours draft, 2026-10-10, from the lore review, picked by the user.)

The campaign is Ra's night journey through the Duat, the underworld of the Book of Gates: twelve hours, each behind
a gate with its guardian. Each zone is one hour: zone 1 the First Hour, zone 12 (the Hall of Judgment, the ankh) the
Twelfth, Ra reborn at dawn. The zones before the lake are the hours of the journey, the lake the gate into the
night, the last two the deep hours and dawn.

* A zone's full name: the hour and its gate, e.g. "The Fourth Hour: the Gate of the Priests". In the zone table.
* Guardians: the Book of Gates names a guardian serpent per gate (e.g. Saa-set, Aqebi, Teka-hra). The zone's boss
  is that hour's guardian in the banner text; the bossless Necropolis names only its gate.
* Shown on entering a zone (the banner, [zone-looks](../zone-looks.md)), in the map and in the save slots.
* Level titles and the journal note per hour: [campaign-texts](../campaign-texts.draft.sonet.md).

## What changes

* A zone table in code (`src/world/campaign.h` or its own file): id, name, first and last level, stone, monster set,
  boss, main mechanic (water, sand, dark, ...). `zoneOf(level)` replaces `desertLevel` and the other level-number
  checks (grep `DESERT_FIRST`, `LEVEL_` ranges, `levelNumber >=`).
* `CAMPAIGN_LEVELS` comes from the table. Until [denser-levels](../denser-levels.draft.opus.md) rebuilds the campaign,
  the table maps today's 35 levels. The target table (40-60 levels) goes in `docs/levels.md` as a design table and
  becomes the code table when the rebuild switches over.
* `docs/levels.md`: a section per zone (levels, story, monsters, mechanic, boss) above the per-level table.
* Glossary: a `zone` row.
* The new bosses go to zones 3, 9 and 10. If they were placed in today's levels first, they move with the rebuild.

## Tests

* Unit: the zones cover 1..`CAMPAIGN_LEVELS` without gaps or overlaps, each boss is in exactly one zone, and the last
  zone ends with the ankh level.
* Today's campaign plays as before (the table maps it 1:1).

## Implemented (2026-10-21)

* `ZONES` in `src/world/campaign.h`: id (`ZoneId`, the twelve target zones), name, gate, first and last level, stone,
  main mechanic, main monsters, boss. It maps today's 35 levels to eight zones; `CAMPAIGN_LEVELS` is its last level.
  `zoneOf(level)` (nullptr outside the campaign) replaced `desertLevel` and `DESERT_FIRST`..`DESERT_LAST`.
* `zoneTitle`: "The Fourth Hour: the Gate of the Priests"; the hour is the zone's `ZoneId` (its place in the target
  table), so the Hall of Judgment is the Twelfth Hour already. The guardians' names are left to the banner
  ([zone-looks](../zone-looks.md)).
* Today's High Priest (lvl34) sits in the necropolis zone until the rebuild moves him to the priests' quarter.
* `docs/levels.md` (Zones): today's table and the target table. Glossary: `zone`.
* Tests: `tests/unit/zones_test.cpp`. No gameplay change: the table maps today's campaign 1:1.
