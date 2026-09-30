# HUD: pre-baked item icons

Status: idea, not started. Follow-up of the [HUD redesign](solved/hud-redesign.md).

## Why

The HUD quick slots draw the inventory's 3D item models small (`drawSlotModel`, `src/ui/player_hud.cpp`). At that
size thin models read poorly: the bow is a faint arc at every turn angle tried (25, 70, 110 degrees), and the spear
is a thin line. The models are also scaled by `Ink::figureScale()`, so they grow 20% in toon mode.

## Idea

Bake one 2D icon per item once, in Blender 5, and draw textured quads instead of the models:

* A Blender script renders each item (club, sword, spear, bow, the potion) at a set angle, transparent background,
  like `tools/blender/render_sheet.py`. Weapons diagonal, as the HUD shows them now (-40 degrees). A thicker or
  outlined look for thin items, so they read at ~40 px.
* Pack them into one HUD atlas (`textures/ui/hud_icons.png`), a fixed grid.
* Potions: one white or grey flask icon tinted with `Inventory::PotionColor`, or one icon per potion.
* `PlayerHud::Slot` takes an atlas cell instead of an `Item*`; the depth-buffer pass in `PlayerHud::draw` goes away.
* Maybe later also for the inventory slots, if the turntable is not wanted there.

## Open questions

* Render resolution per icon (64 or 128 px) and mipmaps, for small windows.
* Keep the 3D models as a fallback when the atlas is missing?
