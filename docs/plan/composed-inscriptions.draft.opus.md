# Composed inscriptions: plates and a hieroglyph sheet instead of baked atlases

Status: draft 2026-10-22, from the user. Needs nothing first.

## Idea

Today `tools/textures/inscriptions.py` bakes every inscription whole (frame, signs, wear) into one cell of
`media/textures/decorations/inscriptions.png`, and the 16 name cartouche pictures into `name_cartouches.png`. The
atlas is a fixed 2048 px, so each new batch of texts shrinks the cells: 256 px at 8 x 8, now 128 px at 16 x 16
(plan campaign-texts, 136 texts). A sign on the wall is about 20 to 40 px, blurry up close and in the journal.

Instead, the game builds each inscription at draw time from parts:

* **Plates.** One texture per style (`cartouche`, `column`, `row`, `stela`): the frame without signs, at full size
  (512 px). The stela's winged sun belongs to its plate.
* **Hieroglyph sheet.** One cell per Gardiner sign in use, drawn once from the Noto font: white fill, ink outline.
  The game tints it with `glColor` (white fill becomes the paint colour, the dark ink stays dark).
* **Layout in C++.** The slot rules (`column_of`, `row`, the stela's right-to-left columns, the sign caps 5 / 7 / 7
  / 12, the colour cycle) move from the tool into a GL-free function in `world/`. It returns the quads: sign, rect in
  the plate's 0..1 space, colour.

Cost grows with the number of distinct signs, not with the number of texts: 111 signs cover all 136 texts today. A
16 x 16 sheet of 128 px cells holds 256 signs at 3 to 6 times today's resolution per sign, and adding a text that
reuses signs costs no texture at all.

## Shape

* **Tool.** `inscriptions.py` writes the plates and the sheet (for example `media/textures/decorations/
  inscription_plates.png`, 2 x 2 cells of 512 px, and `hieroglyphs.png`), plus a manifest of the sheet's cell order
  (`hieroglyphs.txt`, one Gardiner code per line). It collects the codes from every `G:` line in
  `content/inscriptions/` and `content/names/`, sorted, so the order is stable. Same font, `INK`, `PAINT` as now.
* **Loader.** `world/inscriptions.cpp` parses `style:` and `G:` (today only the tool reads them) into the
  `Inscription`. An unknown style is a load error. A code missing from the manifest is a load error too, so a
  stale sheet shows up in `game.log` and in a unit test, not as a blank sign.
* **Drawing.** One function draws an inscription into a rect: the plate quad, then a quad per sign. The wall
  (`dungeon_render_decor.cpp`, `wallQuad`) and the journal page (`JournalScreen::DrawInscription`) both call it.
  The glint (`glintQuad`) runs over the whole rect as before. At most 13 quads per inscription, 2 or 3 per level:
  no FBO needed.
* **Wear.** Today `weather()` knocks holes per inscription with its own seed. Simplest: bake mild wear into the
  plates and the signs, so a sign looks the same everywhere. If that looks too clean, compose each inscription once
  into a texture with `RenderTarget` (`graphics/render_target.h`) and multiply a tiling wear mask with a per-id
  offset. Implementer's choice; try the simple one first and compare screenshots.
* **Outline width.** The outline is baked per cell, so a sign drawn larger has a thicker outline. A stela sign is
  about half a cartouche sign, so check both. If the difference shows, bake the outline thinner and draw a dark,
  slightly larger copy of the sign behind it.
* **Removed.** `inscriptions.png`, `name_cartouches.png`, `INSCRIPTION_ATLAS_GRID`, `NAME_CARTOUCHE_GRID`. Text
  ids stay 1 to 256 (`INSCRIPTION_MAX` becomes a plain limit, no atlas behind it) unless more are needed.

## Name cartouches

The 16 `NAME_CARTOUCHES` groups in the tool go away. Two ways, implementer's choice (I lean to the first):

1. Each name in `content/names/kings.txt` gets a `G:` line that spells it (Khufu: `Aa1 G43 I9 G43`, from its real
   cartouche). The wall shows the true name. A player who reads hieroglyphs can solve the gate without the skill,
   which is a fair reward for that player. `JournalInscription::picture` and `nameCartouchePicture` go; the journal
   page draws from the name's signs.
2. Keep 16 random-looking groups, now as data (a `content/names/` file), still picked per run.

Docs: [names.md](../names.md) "The pictures" and the glossary row "name cartouche" (it says the picture does not
spell the name) follow the choice.

## Tests

* Unit: the layout (caps per style, slots inside the plate, right-to-left stela columns, colour cycle); every `G:`
  code in `content/` is in the manifest; an unknown code or style is a load error with `file:line`.
* Scenario with screenshots: one inscription of each style on the wall, up close, and its journal page, before and
  after. The signs must be sharper and the frames unchanged.

## Docs

[inscriptions.md](../inscriptions.md) (the atlas paragraph, the `style:` and `G:` rows "only the atlas tool reads
it"), [names.md](../names.md), the tool's docstring, [development.md](../development.md#where-is-x). Add glossary
rows for the new names (plate, hieroglyph sheet) once they exist in `src/` (`docs_test.cpp` checks the code column).
