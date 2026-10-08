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

## Open

* Same flask model with per-potion textures, or also a few model variants (taller / ornate bottle for greater ones)?
* Loading: one `Item` per potion kind instead of the shared `items.potion`, or one model with a texture swap.
* Inventory tab icon and HUD slot: drop the tint, use the potion's own texture.
* Keep `PotionDef::colour` for UI text / particles, or drop it.
* Fits the new resistance potions ([resistance-potion.draft.md](resistance-potion.draft.md)), whose "one bottle in
  two colours" would become two textures.
