# Potion textures

Status: draft 2026-10-08.

## Now

Every potion shares one model and one texture: `models/items/potion.md3` + `textures/items/potion.png`, built by
`tools/blender/models/items.py` ([remodeling.md](../remodeling.md)). The texture stays light grey and the engine
tints the whole flask with `PotionDef::colour` (`src/world/items.cpp`, `potionColor()` in `src/ui/inventory.cpp`,
the HUD quick slot in `src/ui/player_hud_view.cpp`). `Assets::Of()` returns the same `Item` for every potion.

## Idea

One texture per potion, the colour baked in, no tint. Frees the texture for detail the tint can't do:

* Greater / stronger potions get richer decoration: gold patterns, hieroglyphs, symbols (e.g. ankh on the Elixir of
  Life, snake on the antidote and resistance potions).
* Lesser ones stay plain, so tier reads at a glance.
* Colour of the glass vs. the decoration (gold, stopper, cord) separate, a tint can only colour everything at once.

## Models (decided 2026-10-08)

Not one flask any more: up to **6 potion models** for now, shaped after Egyptian vessels. Models are shared by
several potions. Set:

| # | Model | Shape | Potions |
|---|---|---|---|
| 1 | Flask | the current round flask, plain clay / faience | Small Health, Small Stamina, Lesser Resistance |
| 2 | Lotus jar | tall slim neck that opens like a lotus flower, two small handles | Large Health, Large Stamina |
| 3 | Pilgrim flask | flat round "New Year flask", short neck, two loop handles | Aphethamine |
| 4 | Canopic jar | squat stone jar, a carved head on the lid (baboon / falcon) | Stone Skin |
| 5 | Cobra vial | slim vial, a rearing cobra coiled around it, the hood forms the stopper | Antidote, Greater Resistance |
| 6 | Ankh flask | ankh-shaped vessel (they existed as ritual vessels), the loop is the bottle neck | Elixir of Life |

* Lesser = model 1 plain; greater = a richer model and more gold / hieroglyphs on the texture.
* Each model keeps about the flask's size and footprint, so it stands on a tile, in a chest and in the inventory slot
  the same way.
* Built in `tools/blender/models/items.py` like the flask, 512 px albedo x AO, one `.md3` per model.
* New potions later reuse one of the 6 models; a 7th needs a new decision.

## Decided (2026-10-08)

* **One asset per model.** Load each of the 6 `.md3` files once, as one `Item` per model, and point each potion kind at
  its model. Don't copy a model per potion. Extra textures only where they add something; the resources must not
  grow without a benefit.
* **Look-alikes on one model.** Potions that share a model look alike on purpose: same vessel and decoration, only
  the colour of the liquid (or a similar small detail) differs.
* **Icons keep the tint.** The inventory slot, the potions tab and the HUD quick slot draw icons, not the model, and
  stay tinted with `PotionDef::colour`. Keep `colour` for the icons only. The 3D model (on the floor, in a chest,
  the inventory detail view) gets no tint any more.
* **Scale.** Refactor freely: each model can have its own scale (floor / chest and inventory detail) instead of the
  single `5` in `loadItem("potion", 5)` and `POTION_DETAIL_SCALE`.
* **Resistance potions** ([resistance-potion.draft.md](resistance-potion.draft.md)): lesser on the flask, greater on
  the cobra vial. Alternative: both on the cobra vial with different textures.
* **Graphics only.** Saves and level files store the potion kind, not the model: no format change.
