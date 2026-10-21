# Zone looks: each zone looks its own

Status: draft 2026-10-21, from the user (the campaign has distinct zones). Needs first:
[campaign-zones](campaign-zones.draft.opus.md) (the zones and their stone in the zone table).

## Idea

A zone should be recognisable at a glance. Today there are two looks: the tombs' limestone and the desert's sandstone
(`src/world/decor.h`, `desertLevel`). Give every zone its own look, read from the zone table.

* **Stone:** wall and floor textures per zone (limestone, sandstone, plus for example dark granite for the
  necropolis, painted plaster for the priests' quarter, wet stone for the cisterns). Procedural textures like the
  existing ones (`tools/textures/`).
* **Decor set:** the props and decals the generator and the editor pick per zone (webs and egg sacs for the spider
  zone, coffins and canopic jars in the necropolis, reeds and lotus in the cisterns). New props follow
  [modeling.md](../modeling.md).
* **Light:** ambient colour and strength, torch density, fog per zone (dark halls for the spider zone, daylight
  shafts in the desert).
* **Sound:** an ambient loop per zone (wind in the desert, drips in the cisterns). Music per zone is optional:
  today there is one soundtrack (`media/sounds/music/soundtrack.ogg`, not tracked by git).
* **Zone banner:** a short title on entering a new zone (`docs/ui.md` look). Shared with
  [duat-hours](duat-hours.draft.sonet.md) if the zones become the hours.

## Tests

* Scenario with a screenshot of one hall per zone (`level gen:` with the zone, or a campaign level per zone).
* Unit: every zone in the table has a stone, a decor set and a light setting.
