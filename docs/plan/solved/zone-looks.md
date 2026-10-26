# Zone looks: each zone looks its own

Status: draft 2026-10-21, from the user (the campaign has distinct zones). Needs first:
[campaign-zones](campaign-zones.md) (the zones and their stone in the zone table).

## Idea

A zone should be recognisable at a glance. Today there are two looks: the tombs' limestone and the desert's sandstone
(`src/world/decor.h`, `desertLevel`). Give every zone its own look, read from the zone table.

* **Stone:** wall and floor textures per zone (limestone, sandstone, plus for example dark granite for the
  necropolis, painted plaster for the priests' quarter, wet stone for the cisterns). Procedural textures like the
  existing ones (`tools/textures/`).
* **Decor set:** the props and decals the generator and the editor pick per zone (webs and egg sacs for the spider
  zone, coffins and canopic jars in the necropolis, reeds and lotus in the cisterns). New props follow
  [modeling.md](../../modeling.md).
* **Light:** ambient colour and strength, torch density, fog per zone (dark halls for the spider zone, daylight
  shafts in the desert).
* **Sound:** an ambient loop per zone (wind in the desert, drips in the cisterns). Music per zone is optional:
  today there is one soundtrack (`media/sounds/music/soundtrack.ogg`, not tracked by git).
* **Zone banner:** on entering a new zone, the hour and its gate, and its guardian on a boss zone
  ([campaign-zones](campaign-zones.md#the-twelve-hours-of-the-duat)), `docs/ui.md` look. On every other
  level start a smaller banner with the level's title ([campaign-texts](../campaign-texts.md)).

## Tests

* Scenario with a screenshot of one hall per zone (`level gen:` with the zone, or a campaign level per zone).
* Unit: every zone in the table has a stone, a decor set and a light setting.

## Implemented (2026-10-21)

* `ZONE_LOOKS` (`src/world/zone_look.h`, all twelve zones): decor set (a factor per prop and decal on the tier
  weights), ambient light, haze (fog in the lighting shader past `FOG_START`), torch chance, ambient loop, guardian.
  `DEFAULT_LOOK` outside the campaign (the old tombs' look).
* Stone: wet stone, basalt and granite walls (plain and worn), floors and ceilings (`tools/textures/surfaces.py`);
  plaster is limestone painted at least 70%; the built stones have at most 15% rough rows.
* Sound: `tools/audio/zone_ambience.py`, 10 s loops in `media/sounds/ambience/`, on a reserved channel at the effects
  volume (`Ambience`). No music per zone (optional).
* Banner: `ZoneBanner` with `levelBanner`: the hour and gate, the zone, the guardian on a zone's first level; "Level N"
  and the zone on the others, until [campaign-texts](../campaign-texts.md) gives the levels titles.
* Not in today's campaign, so not seen in the game yet: the web halls' darkness, the Lake of Fire's basalt and red
  light, the priests' quarter, Sokar. No new props (egg sacs): the web halls lean on the web and skeleton.
* Tests: `tests/unit/zone_looks_test.cpp`, `tests/scenarios/solved/zone_looks.txt` (a hall per zone, the banners).
* The zone is the level's own (user, 2026-10-21): level format v5 has a `zone SLUG` line (`LevelMeta`, `ZONE_INFO`
  in `campaign.h` by `ZoneId`: slug, name, gate, stone, mechanic), the editor sets it (zone button, `Z`), save games
  keep it. The game dresses, lights and sounds a level by its file's zone, not its number; a unit test checks each
  campaign file names its range's zone. `zone none` (test, generated levels): the tombs' look.
* Sand edge (user, 2026-10-21): a sand floor beside a stone one met in a hard seam (granite, wet stone). The stone
  floor next to it now gets `floor_sand_edge` over it (`SurfaceCell::sandLeft` / `sandRight`), sand thinning out
  by about half the cell.
