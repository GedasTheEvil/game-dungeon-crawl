# UI screens

The full-screen UI (menu with its sub-screens, inventory, riddle, draft map, journal) shares one Egyptian look:
carved tomb wall, dark panels with bronze and gold frames, lapis and stone tiles, papyrus text. New screens reuse the
parts below instead of drawing their own.

| Screen | Code | Canvas |
|---|---|---|
| Main / in-game menu, Save, Load, Options, Credits | `src/ui/menu.cpp` (`MainMenu`) | 160 x 100 |
| Inventory | `src/ui/inventory.cpp` | 160 x 100 |
| Riddle | `src/ui/riddle.cpp` | 160 x 100 |
| Draft map | `src/ui/map_view.cpp` | 100 high, width follows the window |
| Journal | `src/ui/journal_view.cpp` (data: `src/world/journal.h`) | 160 x 100 |
| Level gem (HUD badge) | `src/ui/level_gem.cpp` | own scale, not a screen |
| Player HUD (health, stamina, quick slots, keys, XP) | `src/ui/player_hud.cpp` | 100 / `SCALE` high, width follows the window; over the game |
| Boss bar | `src/ui/boss_bar.cpp` | 100 high, width follows the window; moves above the player HUD when they would overlap |
| Status box (gameplay message) | `src/ui/status_box.cpp` | 100 high, width follows the window; over the game |
| Win / death screen | `src/ui/end_screens.cpp` (`EndScreens`) | legacy: a textured quad (`ui/win.png`, `ui/dead.png`) in the 3D scene, not the shared look |

Shared drawing code: `src/ui/ui_draw.h` (namespace `ui`). The level editor (`tools/editor`) uses it too.

## Canvas

* Layout is in canvas units on a **160 x 100 canvas, y up** (`ui::CANVAS_W`, `ui::CANVAS_H`). It keeps its aspect
  ratio and is centred;
  `ui::visibleArea()` returns the part the window shows, margins included. Set it up with
  `glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200)`.
* The HUD parts and the draft map use a canvas of fixed height and the window's aspect instead:
  `ui::beginSquareCanvas(height, resX, resY)` sets it up and returns its width.
* Fonts: `ui::loadScreenFonts()` loads a screen's title, heading, body and small fonts. The feedback line under a
  screen is a `ui::Toast` (`Show`, `Alpha`).
* Mouse to canvas: `ui::toCanvas()`. Hit tests use `Rect::contains` on the same `constexpr Rect`s the drawing uses.
* Only the backdrop fills the margins (`area`); all panels, buttons and text sit inside 0..160 x 0..100.
* Line widths are in pixels, sizes in canvas units.
* Layout lives in `constexpr Rect` / `float` constants at the top of the file (`menu.cpp` and `status_box.cpp` mark it with a `// ---- layout ----` comment).
  Repeated items get a `xxxRect(index)` function.

Vertical bands used by every 160 x 100 screen:

| y | Part |
|---|---|
| 91.5 | title rule |
| 88 | title baseline |
| ~13–85 | panels |
| 9.5–17.5 | Back button (menu sub-screens); toasts at y 7–12 (Options: under the Back button, baseline 5.6) |
| 2.2 | key hint footer |

## Frame order

A screen's `Draw()`:

1. `glClear`, projection as above, `glDisable(GL_DEPTH_TEST)`.
2. Backdrop, title, panels (`DrawBackground`).
3. Contents: shapes and text, switching with `beginShapes()` / `beginText()`.
4. 3D models, if any (inventory): `glClear(GL_DEPTH_BUFFER_BIT)` first, so models never cut into the flat UI.
5. Footer (hint + toast).
6. Restore state (`glDisable(GL_BLEND)`, `glEnable(GL_TEXTURE_2D)`, `glEnable(GL_DEPTH_TEST)`, white colour).

The screen does not end the frame: `Draw()` (`graphics/draw.cpp`) calls the screen that is open (`GameState::ui.screen`,
one at a time) and then flushes, lets the scenario runner take its screenshot and swaps the buffers.

## Parts

### Backdrop

`ui::backdrop(area, textures.loadingBackground.ID())`: the carved wall over the whole `area`, tinted
`{0.34, 0.27, 0.20}`, and a black vignette 22 wide. The editor passes its own wall texture; the draft map draws its
own (vignette 20, canvas without margins).

### Title

`ui::titleBar(title, 80, caption, reach)`: caption in `GOLD` at baseline 88, a `GOLD_DIM` 2 px rule either side
at y 91.5 with big diamonds at the ends and small ones next to the text. Reach from centre: 44 for a narrow panel
(main menu, credits) and the screens with tabs (inventory, journal), 58 for the menu's wide sub-screens, 72 on the
riddle.

### Panel

`ui::panel(rect, 0.9f)`: brown gradient fill, 3 px `BRONZE` frame, 1 px `GOLD_DIM` inner line inset 1.1,
`GOLD` corner studs. The main container of a screen. Standard rects:

* Menu buttons: `{46, 19, 68, 64}`
* Save / Load slots: `{12, 22, 136, 62}`
* Options: `{6, 20, 148, 64}`
* Inventory items `{4, 13, 92, 72}` + details `{100, 13, 56, 72}`

### Picture frame

For an image (credits sheet, riddle gate): `texturedRect()` (leaves texturing off, call `beginShapes()` after),
then `strokeRect(r, BRONZE, 1, 3)`, `strokeRect(r.inset(1.1), GOLD_DIM, 0.8, 1)` and `cornerStuds()`.
Optional drop shadow before it: black rect offset `(+0.6..0.8, -0.9..1)`, alpha 0.45–0.5. Keep the image aspect
ratio; never stretch a picture over the window. Credits: the 512 x 512 sheet at 60 x 60 inside a panel 3 units bigger.

### Papyrus scroll

Light surface for text to read (inventory details, riddle): `textures.papyrus` with UVs `0.04, 0.07 – 0.96, 0.93`
(crops the torn edges), 3 px `BRONZE` frame, corner studs, no inner gold line. Text on it uses the ink colours
(`INK`, `INK_RED`, `INK_FADED`), rules in `INK_FADED` with an `INK_RED` diamond.

### Tiles (buttons, slots)

`ui::tile(rect, style, hovered, held)` draws every button and slot and returns the rect as drawn (moved down
`TILE_SINK` while held). Put the label on the returned rect:

| State | Look |
|---|---|
| Idle | black drop shadow `(+0.5, -0.7)` alpha 0.4, gradient fill, 1 unit white top highlight alpha 0.08 |
| Hover | additive `GOLD` glow `ring(r, 2.2, GOLD, 0.4, 0)`, lighter fill, `GOLD_BRIGHT` 2.5 px frame, highlight 0.16 |
| Held | no shadow, sinks 0.4, darker fill, no highlight |
| Disabled | flat dark fill alpha 0.9, `BRONZE` frame alpha 0.6, no hover |
| PapyrusDisabled | faded tan tile on a papyrus scroll, `INK_FADED` frame; label in `body` `INK_FADED` |

Styles:

* **Lapis**: primary action (New Game, Return to Game, active tab, inventory Equip / Upgrade / Drink).
  `LAPIS`→`LAPIS_DARK`, `GOLD` 2.5 px frame, `GOLD_DIM` inner line.
* **Stone**: all other actions and the save slots. `STONE_TOP`→`STONE_BOTTOM`, `BRONZE` 1.5 px frame; the `GOLD_DIM` inner line only on hover.

One lapis button per group. Menu buttons are 56 x 9, 11.5 apart, centred in the panel; the in-game menu's six are
10.2 apart. Back is `{63, 9.5, 34, 8}`.
Label colour: `GOLD` (lapis) or `LABEL` (stone), `TEXT_HOVER` when hovered.
An action fires on mouse **up** over the same target it went down on (`pressed` / `hovered`).

Content that covers the tile edges (the inventory slots' name band) is followed by `ui::tileFrame()` to draw the
frame again on top. Things drawn in a later pass (the slot models and labels) move down `TILE_SINK` too while held.
The level editor still has its own smaller buttons (`Editor::drawButton`).

### Icon well

Dark square on the left of a tile: `ui::iconWell(tile, inset)` returns its rect; `menu.cpp` draws the `WELL` fill (lapis on a lapis tile), `GOLD_DIM` frame
(`GOLD` on hover). Holds a flat vector glyph from `drawIcon()` (`menu.cpp`, `enum class Icon`: Play, Save, Load, Gear, Ankh, Exit, Pyramid, Back)
or a number (save slots, lapis seal when the slot is used). Glyphs are shapes, not textures.

### Selection

Selected, not just hovered, items (inventory slots) get a breathing gold halo
(`ring`, alpha `0.35 + 0.25 * pulse`, additive) and a 3 px `GOLD` frame.

### Section heading

`heading` font in `GOLD` at the top left of a panel section, a 1 px `BRONZE` rule from after the text to the panel's
inner edge. Table headers: `small` font in `LABEL`, rule under them, rows striped
with white alpha 0.035 (Options).

### Options rows and key cells

Options tabs: Controls, Display and Sound.

* Display and Sound: one striped row a setting (`DISPLAY_ROWS`, `SOUND_ROWS` in `menu.cpp`): name in `body`
  `LABEL`, what it does in `small` `LABEL_DIM`, the control on the right. A row's kind picks the control: a switch
  (On / Off tile, lapis when on), a choice (stone tile with the value, a click steps to the next; disabled when it
  does not apply) or a slider (dark track filled lapis up to a gold knob, the number in `heading` to its right; it
  follows the mouse while the button is down and saves when it comes up). A new setting is a field in `Settings`
  (`src/state/settings_ini.h`), a key in the parser and writer, and a row.
* Controls: two columns of rows, an action and three cells (two keys, the mouse button); the "Reset to defaults"
  stone tile ends the right column, the fixed keys are named in a `small` line under the table. A click on a cell
  waits for the input: the cell pulses lapis with "Press" (or "Click"), the footer says what it waits for.

The choices are `GameState::settings`, kept in `saves/settings.ini` ([settings.md](settings.md)).
`GameState::ApplySettings(true)` hands them to the renderer and the mixer and writes the file.

Key cells: a dark gradient cap with a `GOLD_DIM` frame (`GOLD` when hovered) and a black shadow, the key cap name in
`small` `GOLD`, a `LABEL_DIM` dash when unbound; the mouse cell shows a small mouse with its button lit.

### Footer and toast

* Key hint: `textCentered(small, 80, 2.2, hint, LABEL_DIM)`, keys and actions separated by 4 spaces
  (`"Click a slot to load    Esc: back"`).
* Toast: `body` font, `{1, 0.9, 0.6}`, above the footer, shown 2200 ms and faded out over the last 600 ms.

### Book (journal)

A cloth-bound field notebook ([plan/journal-real-book.md](plan/solved/journal-real-book.md)): grey cloth cover
(`textures/ui/journal_cloth.png` tinted `CLOTH`) showing all round, a cream elastic strap (`STRAP`) round its left edge,
red and white headbands at the spine, the page block's edges either side (more on the side the book is thicker). Two
white grid pages (`textures/ui/journal_paper.png`, the spine shading baked in, mirrored on the left page); both textures
come from `tools/textures/journal_book.py`. One ribbon bookmark per section (`INK_RED` creatures, `LAPIS` riddles,
`GOLD_DIM` field notes, swallowtail end) marks the section's first page
([plan/journal-ribbons-change-sides.md](plan/solved/journal-ribbons-change-sides.md)): it hangs out of the right edge
while the section lies ahead, out of the left edge (mirrored, over the strap) once it is open or passed, coming out from
under the pages on top of it (the open left page's own ribbon lies on it), further out the more pages lie between it and
the open spread (`EDGE_STEP` per page edge). Its letter (M, R, F) in `PAPER` on the open and the hovered one; the
hovered one shows its name on a dark label on its page. Page turn arrows in `PENCIL` in the bottom corners.

On the pages: page numbers stamped in `INK_RED` in the top outer corner (`fonts/courier.png`), the level pencilled
above; underlined headings and the text in `INK_BLUE`; pencil (`PENCIL`, alpha 0.7) for what matters less (hints,
captions of sketches). Creature pages have a margin column of labels (`SEEN`, `SAW`, `KILLED` in red, `TRIED`) and a
form filled in as it is learnt (`HP = ~40`, a blank line while unknown).

Creature pictures: the model's move clip, frame 0, from the side and a little above. Before the kill a pencil sketch
on the grid: a depth pass, then the back-face edges as `PENCIL` lines, front faces culled. After it a black and white
photo pasted in a little askew: white border, dark backdrop, the model lit by a fixed-function light with a grey copy
of its texture (`Texture::LoadPNG`'s pixel filter), a red catalogue number above (`L07-03`: level, page), a caption
with an arrow below.

Page turn: each page is drawn into a `RenderTarget` (the page's size in window pixels), then laid on the book or bent
by `drawPageCurl` (`src/ui/page_curl.h`): a mesh folded along the bisector of the bottom free corner and where it is
pulled, round a cylinder, the back showing the next page, a shadow on the page under it. A click or key swings the
corner over in 650 ms, a drag follows the mouse (let go past the spine: over, before it: back), a hovered corner
lifts a little. A ribbon or Up / Down to another section turns one page in its direction; the ribbons whose first
page it crosses ride on it to the other side (drawn into the page's texture, which reaches `RIBBON_OVERHANG` past the
free edge, `PageCurl::overhang`), bending with the curl. Sound:
`sounds/ui/page_turn.wav` (`tools/audio/page_sound.py`).

### Screen tabs

Inventory, draft map and journal share a tab strip at the top right (`ui::screenTabs`, game side
`src/ui/screen_tabs.cpp`): three tiles 9.5 x 7 centred on the title rule, x 127.5–158, a flat icon (chest, map sheet,
open book) and the key (I, M, J) in `small` `GOLD`. The open screen's tile is lapis, the others stone; the hovered one
shows the screen's name on a dark label to the left. Screens with the strip keep their title rule short
(`SCREEN_TABS_TITLE_REACH`, 44). The strip is always drawn in the 160 x 100 canvas, also over the draft map's
window-wide one, so it stays in the same place. Its clicks are handled before the screen's own (`ScreenTabs::Mouse`).

### Inventory group tabs

The inventory's items panel has one tab per `ItemGroup` (weapons, potions, amulets, rings) across its top, in place
of section headings: four tiles 19.75 x 6 at y 77.6, as wide as the slot grid. Each has a flat icon from the HUD atlas
(`PlayerHud::drawIcon`: sword, flask tinted red, amulet, ring) and the group name in `small`. The open tab is lapis,
the others stone. A group with nothing found yet (`ItemBag::AnyFound`) is `TileStyle::Disabled` with a dimmed icon,
takes no clicks, and on hover shows "<name>: none yet" on a dark label under it.

The slots are one grid for every tab: 4 a row, 19 x 22 each, one model scale (`SLOT_SCALE`), rows from the group's
item count in `ItemKind` order. Two rows fit; a group with more needs scrolling by whole rows (not built yet). An
item never found shows a grey question mark instead of its model (slot and details) and nothing else that tells what it
is: no name in the slot, "Unknown" with no type, stats or lore in the details. One found and used up keeps the dark
silhouette. The number row keys still pick a slot in `ItemKind` order and open its tab.

### Status box

The gameplay message (`Game().ShowStatus`) over the running game: a `panel` (fill alpha 0.92) sized to the text,
min 40 wide, top edge at y 86, centred, with the picture-frame drop shadow. `status` font in `GOLD`, one line per
`'\n'`, 6 apart. Fades in over 150 ms and out over the last 500 ms of `STATUS_MS`; `panel`'s `frameAlpha` fades the
frame and studs with it.

### Player HUD

A `panel` bottom left (`PlayerHud::PANEL`) over the running game, with the status box's drop shadow. Bars: dark
trough, gradient fill, 1.5 px `GOLD_DIM` frame and `GOLD` end diamonds (the boss bar's look). Quick slots are
`TileStyle::Stone` tiles with a flat item icon (`textures/ui/hud_icons.png`, from `tools/textures/hud_icons.py`), a
count badge like the inventory slots and a key cap under each like the options table. Flashes and pulses are additive
rings (`ring` with `GL_SRC_ALPHA, GL_ONE`). Details: [plan/solved/hud-redesign.md](plan/solved/hud-redesign.md),
icons: [plan/solved/hud-icons.md](plan/solved/hud-icons.md).

## Colours

Palette constants in `ui_draw.h`; use them, don't write new RGB values.

| Use | Constant |
|---|---|
| Titles, headings, primary text, lapis frame | `GOLD` |
| Hover frame and text | `GOLD_BRIGHT` frame, `TEXT_HOVER` text `{1, 0.92, 0.65}` |
| Rules, inner frame lines | `GOLD_DIM` |
| Outer frames, stone tile frame, section rules | `BRONZE` |
| Primary tile fill | `LAPIS` → `LAPIS_DARK` |
| Stone tile fill | `STONE_TOP` → `STONE_BOTTOM` |
| Tile hover / held fills | `LAPIS_HOVER_*`, `STONE_HOVER_*`, `*_HELD_BOTTOM` |
| Hovered label | `TEXT_HOVER` |
| Panel fill | `PANEL_TOP` → `PANEL_BOTTOM` |
| Text on papyrus | `INK`, `INK_RED` (warnings, reward), `INK_GREEN`, `INK_FADED` (hints, disabled) |
| Pencil: draft map, journal minor notes | `PENCIL` (alpha 0.7 for minor notes) |
| Journal: ink, paper, strap, cover cloth | `INK_BLUE`, `PAPER`, `STRAP`, `CLOTH` |

`menu.cpp` adds `WELL` (icon well fill), `LABEL` `{0.86, 0.72, 0.47}` for body text on dark panels and `LABEL_DIM` `{0.55, 0.45, 0.30}` for
secondary text and footers.

## Fonts

All text uses `fonts/papyrus.png` (printable ASCII only, no accents: write "Skucas", not "Skučas"), except what the
archaeologist writes by hand on the journal's pages: `fonts/kalam.png` (Kalam, SIL OFL, `fonts/kalam-OFL.txt`) in
`INK_BLUE` / `PENCIL`, and the journal's stamped page and catalogue numbers: `fonts/courier.png` (Courier 10 Pitch,
Bitstream, `fonts/courier-LICENSE.txt`). The journal's buttons stay papyrus. A font
sheet comes from a TrueType font with `python3 tools/textures/font_sheet.py FONT.ttf fonts/NAME.png` (same grid,
baseline and cap height as the papyrus sheet).
Each screen loads four sizes on its first frame (fonts need the GL context):

| Font | `Load(file, size, spacing, true)` | Use |
|---|---|---|
| `title` | 8, 0.3 (menu) / 7, 0.3 (inventory, riddle) | screen title, slot numbers |
| `heading` | 5, 0.16 | button labels, section headings |
| `body` | 3.6, 0.1 | body text, toasts |
| `small` | 3, 0.08 | footer, table text, captions |

The y passed to `text()` is the baseline. The font sheets have no alpha, so `ui::text()` draws twice (cut out, then
add the colour); call it only after `beginText()`, go back with `beginShapes()`.

## Menu sub-screens

Save, Load, Options and Credits are flags on `MainMenu` (`saveD`, `loadD`, `optionsD`, `creditsD`). A new one needs:

1. A flag, added to `InSubScreen()` and cleared in `ResetSubScreens()` (Esc then backs out of it by itself).
2. A branch in `MainMenu::Draw()`: `DrawBackground(caption)`, the contents, `DrawBackButton()`, `DrawFooter(hint)`.
3. Its panel rect in `DrawBackground()`, and the title rule reach (`wide`).
4. Its targets in `TargetAt()`; at least `BACK_BUTTON` → `BACK`.
5. A scenario in `tests/scenarios/` with screenshots of the screen and the hovered Back button.

Scenario mouse coordinates are percent of the window, y from the bottom. With a 16:9 window the canvas fills the
height, so canvas y = percent y, and canvas x 80 is 50 %. Back button: `click 50 13`.
