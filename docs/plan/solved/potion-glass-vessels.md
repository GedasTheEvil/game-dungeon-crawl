# Potions: see-through glass vessels, the liquid its own model

Status: solved 2026-11-03 (play-tested by the user). Was: draft 2026-11-01, needs nothing first. From the user.

Opus: new Blender models (the glass vessels split in two) and an engine change to the item's second part.

## Idea

The glass potion vessels look like painted clay today: one opaque model, the liquid colour baked into its texture
(`POTIONS` in `tools/blender/models/items.py`, [potion-textures](potion-textures.md)). Make the glass slightly
see-through, with the liquid a model of its own inside it. The liquid stands still (no slosh, no level change).

**Focus: the inventory** (slot and card, the user's priority). Tune the look there first: inventory light, slot size,
the card's close-up. On a chest it only has to look no worse than today; no extra work for it.

## Lore

Fits, with one care point. Egypt made glass vessels from about 1500 BC (Thutmose III, Amarna): small core-formed
flasks and jars for oils and kohl, the same use as the potions. That glass was coloured and cloudy, not clear: clear
blown glass is Roman. The game already calls the flask and the lotus jar glass ([remodeling.md](../../remodeling.md)), so
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
  model, the glass as its second part (`Item::Part`, [ring-accessories](ring-accessories.md)), so no third
  part is needed: the liquid is opaque and draws with the first model.
* Textures: the liquid's colour per potion stays in the first model's texture as now (`liquid` / `liquid_dark`
  overridden per potion), or becomes one liquid texture with a tint: the implementer's choice. The glass: one
  texture per vessel, tint and alpha on the part.
* Engine: today the second part is drawn as a ring's gem (`Item::drawPartScaled`, `src/entities/item.cpp`):
  emissive and with a glint. Glass is lit by the scene like any model and does not glint; at most a faint highlight
  (the metallic-shading shine, [metallic-shading](metallic-shading.md), if it fits glass). Give the part a
  style (gem / glass), set in `loadPart`.
* `loadPotions` (`src/state/assets.cpp`) loads the glass part once per vessel and shares it (`sharePart`) between its
  potions.
* Everywhere a potion draws: the inventory slot and card (`DrawPart` calls in `src/ui/inventory.cpp`), on a chest
  (`Dungeon`'s see-through pass, `dungeon_render.cpp`), the HUD quick slots if they draw the model, and the model
  viewer. On a chest, a quick look only (the gold heap hides the lower half; a seam through the glass there is
  acceptable if the fix is not trivial).
* The icons (`PotionDef::colour`) do not change.
* Screenshot scenario: every potion in the inventory slots and on the item card.

## Done

* Models (`items.py`): `glass_vessel` builds the flask, the lotus jar and the cobra vial (`GLASS_VESSELS`) as the
  opaque bits plus the liquid (a closed lathe up to the shoulder, `GLASS_THICK + GLASS_GAP` inside the glass) and a
  glass shell `potion_<vessel>_glass.md3`. The shell is one surface (one layer of glass over the liquid; a doubled wall
  washed the colours out), thick only at the flask's rolled lip; the lotus's and the cobra's mouths are under the
  faience flare and the gold lip. The lotus's glyph band is now a painted strip over the glass, its mouth dark
  (`enamel`: the liquid stands at the shoulder). The potions' textures stay per potion (`liquid` / `liquid_dark`).
* Glass: `glass_shell` pale blue-green, metal 0.8 (the faint shine of metallic-shading), a 128 px texture per vessel,
  drawn at `POTION_GLASS_ALPHA` 0.32: cloudy, the liquid's colour reads through it.
* Engine: `PartStyle` (`Gem`, `Glass`) on `Item::Part`, set in `loadPart`; glass is lit by the scene, no emissive,
  no glint. `PotionModelDef::glass` names the shell; `loadPotions` loads it once per vessel and shares it
  (`Item::sharePart`). The inventory slot and card and the chest (`drawItemParts`) already draw the second part;
  the HUD quick slots draw icons, no model. The model viewer draws the shell over a glass vessel in the lit modes.
* Ankh: stays opaque (reads as faience, the safer choice). The optional trailed threads: left out.
* On a chest: no seam work needed (the shell is drawn after the opaque scene, over the gold heap).
* Tests: `tests/unit/items_test.cpp` (the glass vessels have a shell, its files exist), `metal_mask_test.cpp` (the
  shells shine); scenario `tests/scenarios/solved/potion_glass_vessels.txt` (every potion in the slots and on the card, a
  flask, lotus and cobra on their containers). Play-tested by the user (its play script removed).
