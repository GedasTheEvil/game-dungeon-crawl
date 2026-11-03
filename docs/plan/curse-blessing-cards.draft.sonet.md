# Curse and blessing cards: painted backgrounds

Status: draft 2026-11-03, needs [shrine-of-isis](shrine-of-isis.draft.opus.md) first (its yellow Isis card is one
of the two backgrounds). From the user.

## Idea (from the user)

The HUD's curse and blessing tiles are drawn in code now. Paint them to a texture instead. Each tile has two parts:
a background and an icon on top of it. Every curse has the purple background (as now), every blessing the yellow one
(as the Protection of Isis). The icon is composed on top. Not a big task, but it needs a new texture sprite.

## Now

`src/ui/player_hud.cpp`:

* `drawCurses`: drop shadow, a purple gradient frame (`CURSE_FRAME_TOP/BOTTOM`, `fillRect`), a `TileStyle::Stone`
  tile, the curse's icon from `hud_icons.png` (`Icon::FirstCurse + c`, cells 37..44), one or two gold pips (`diamond`).
* `drawIsisWard`: drop shadow, a flat yellow card (`ISIS_TOP/BOTTOM`, `fillRect`), the throne icon (`Icon::IsisWard`).
* `drawWaterBlessing` (Blessing of Hapi): a stone tile with a code-drawn blue-green drop (`drop`), no frame.

The icons are already painted (`tools/textures/hud_icons.py`). Only the backgrounds (and Hapi's drop) are code.

## Plan

* New generator `tools/textures/status_cards.py` (PIL, the `hud_icons.py` style: drawn at 4x, downsampled, `bleed`)
  writes `media/textures/ui/status_cards.png`: one row of cells, 128 px each:
  * cell 0, curse: a purple card. The current look, painted: a dark-purple frame with a bevel, a stone or dark
    purple face with some grain, so it does not read as a potion tile.
  * cell 1, blessing: a yellow-gold card, the same shape and frame width: the Isis card's colours (`ISIS_TOP/BOTTOM`),
    a light bevel, a warm face.
  * Leave the face's middle calm (the icon goes there) and the bottom strip clear for the pips.
* Load it in `src/state/assets.cpp` beside `hudIcons` (new `Texture statusCards` in `texture_registry.h`).
* HUD: one helper, `statusCard(rect, CardKind::Curse|Blessing)`, draws the background cell over the tile rect (drop
  shadow stays in code), then the icon as now. `drawCurses` and `drawIsisWard` use it; remove the `fillRect` frames,
  the stone tile under curses and the then-unused colour constants.
* Blessing of Hapi: a blessing too, so the yellow card. Paint its drop as an icon in a free `hud_icons.py` cell
  (cell 5, new `Icon::HapiBlessing`; cells 5..7 are empty), the potion's blue-green (`WATER_DROP`), and drop the
  code-drawn `drop` for it.
* Pips (lesser / greater) and the seconds-left labels stay code-drawn, unchanged.
* Docs: `docs/ui.md` (HUD tiles), the comment block in `player_hud.h`, the glossary rows "curse" ("Purple-framed HUD
  tiles") and "Protection of Isis" if wording changes. Add a "blessing card" / "curse card" row to the glossary.
* Test: a scenario in `tests/scenarios/` with two curses (one greater), the Isis ward and the Hapi blessing running,
  one screenshot of the HUD row. Check the icons sit centred on the cards and the pips read on both colours.

## Open points (implementer's choice)

* Which effects count as blessings: Hapi and Isis for sure. The resistance potion and poison stay plain stone tiles
  (potion effects), the daze keeps its sand-gold frame (neither curse nor blessing) unless it looks out of place
  next to the new cards; then give it a third card cell.
* Card corner shape (square as now, or slightly rounded / cartouche-like): keep it close to the current tile size so
  the HUD layout (`poisonRect`) does not change.
