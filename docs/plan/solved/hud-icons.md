# HUD: item icons

Status: implemented 2026-09-30, verified in play 2026-09-30. Follow-up of the [HUD redesign](hud-redesign.md).

## Why

The HUD quick slots drew the inventory's 3D item models small (`drawSlotModel`, `src/ui/player_hud.cpp`). At that
size thin models read poorly: the bow is a faint arc at every turn angle tried (25, 70, 110 degrees), and the spear
is a thin line. The models are also scaled by `Ink::figureScale()`, so they grew 20% in toon mode.

## Decision

Not a bake of the item models: small drawn icons that stand for the item (a bow, a sword, a spear), made to read at
~40 px. Flat glyphs: flat fill, an ink outline round the whole icon and thin ink lines between the parts, no shading.
The inventory keeps its turning 3D models.

## Implementation

* `tools/textures/hud_icons.py` (PIL, like `decals.py`, reuses its `bleed`) writes `textures/ui/hud_icons.png`:
  4 x 2 cells of 128 px, drawn at 4x. Cells: club, sword, spear, bow, potion. The weapons lie diagonally, grip
  bottom left. The flask is white and grey; the HUD tints it with `Inventory::PotionColor`.
* `PlayerHud::Slot` holds a `PlayerHud::Icon` (atlas cell, `None` = empty slot) instead of an `Item*`.
  `drawSlotIcon` draws a textured quad; the depth-buffer model pass in `PlayerHud::draw` is gone.
* `weaponIcon` (`src/graphics/draw.cpp`) maps the equipped weapon to its icon. Texture: `TextureRegistry::hudIcons`.
* Test: `tests/scenarios/hud_icons.txt`, one screenshot per weapon with two tinted potions.

## Verify in play

* Each weapon (keys 1-4) shows its icon, the potions show their colour.
* Icons stay sharp at small and large window sizes (mipmaps), and don't change size in toon mode.
