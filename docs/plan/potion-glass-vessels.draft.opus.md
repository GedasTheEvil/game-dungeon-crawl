# Potions: see-through glass vessels, the liquid its own model

Status: draft 2026-11-01, needs nothing first. From the user.

Opus: new Blender models (the glass vessels split in two) and an engine change to the item's second part.

## Idea

The glass potion vessels look like painted clay today: one opaque model, the liquid colour baked into its texture
(`POTIONS` in `tools/blender/models/items.py`, [potion-textures](solved/potion-textures.md)). Make the glass slightly
see-through, with the liquid a model of its own inside it. The liquid stands still (no slosh, no level change).

## Lore

Fits, with one care point. Egypt made glass vessels from about 1500 BC (Thutmose III, Amarna): small core-formed
flasks and jars for oils and kohl, the same use as the potions. That glass was coloured and cloudy, not clear: clear
blown glass is Roman. The game already calls the flask and the lotus jar glass ([remodeling.md](../remodeling.md)), so
only the look changes:

* Glass: lightly tinted (pale blue-green, the natural tint of old glass), only slightly see-through, cloudy rather
  than clear. The liquid's colour must still read through it: it tells the potions apart.
* Optional, as on real core-formed flasks: a few trailed glass threads (yellow / white zigzags) in the opaque part.
* Faience and alabaster stay opaque: the pilgrim flask (faience) and the canopic jar (alabaster). Ankh: the
  implementer's choice (an ankh-shaped glass flask is fantasy either way; faience is the safer read).

Vessels to split: `flask` (small health, small stamina), `lotus` (large health, large stamina), `cobra` (antidote,
lesser and greater resistance; its vial becomes glass too, the gold cobra stays opaque).

## Plan

* Models: each glass vessel becomes three meshes on the same origin: the opaque bits (cork, cord, gold foot, faience
  petals, cobra), the liquid (a closed mesh up to the shoulder, a little smaller than the glass's inside), the glass
  shell (with some thickness at the rim, so the mouth reads). Export: the opaque bits and the liquid as the item's
  model, the glass as its second part (`Item::Part`, [ring-accessories](solved/ring-accessories.md)), so no third
  part is needed: the liquid is opaque and draws with the first model.
* Textures: the liquid's colour per potion stays in the first model's texture as now (`liquid` / `liquid_dark`
  overridden per potion), or becomes one liquid texture with a tint: the implementer's choice. The glass: one
  texture per vessel, tint and alpha on the part.
* Engine: today the second part is drawn as a ring's gem (`Item::drawPartScaled`, `src/entities/item.cpp`):
  emissive and with a glint. Glass is lit by the scene like any model and does not glint; at most a faint highlight
  (the metallic-shading shine, [metallic-shading](solved/metallic-shading.md), if it fits glass). Give the part a
  style (gem / glass), set in `loadPart`.
* `loadPotions` (`src/state/assets.cpp`) loads the glass part once per vessel and shares it (`sharePart`) between its
  potions.
* Everywhere a potion draws: the inventory slot and card (`DrawPart` calls in `src/ui/inventory.cpp`), on a chest
  (`Dungeon`'s see-through pass, `dungeon_render.cpp`), the HUD quick slots if they draw the model, and the model
  viewer. Check that the chest's gold heap still hides the vessel's lower half without a seam through the glass.
* The icons (`PotionDef::colour`) do not change.
* Screenshot scenario: the potions in the inventory and on a chest, lit and in the dark.
