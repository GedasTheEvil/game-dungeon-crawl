# UI screens

The full-screen UI (menu with its sub-screens, inventory, riddle, draft map) shares one Egyptian look: carved tomb
wall, dark panels with bronze and gold frames, lapis and stone tiles, papyrus text. New screens reuse the parts below
instead of drawing their own.

| Screen | Code | Canvas |
|---|---|---|
| Main / in-game menu, Save, Load, Options, Credits | `src/ui/menu.cpp` (`MainMenu`) | 160 x 100 |
| Inventory | `src/ui/inventory.cpp` | 160 x 100 |
| Riddle | `src/ui/riddle.cpp` | 160 x 100 |
| Draft map | `src/ui/map_view.cpp` | 100 high, width follows the window |
| Level gem (HUD badge) | `src/ui/level_gem.cpp` | own scale, not a screen |

Shared drawing code: `src/ui/ui_draw.h` (namespace `ui`). The level editor (`tools/editor`) uses it too.

## Canvas

* Layout is in canvas units on a **160 x 100 canvas, y up**. It keeps its aspect ratio and is centred;
  `ui::visibleArea()` returns the part the window shows, margins included. Set it up with
  `glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200)`.
* Mouse to canvas: `ui::toCanvas()`. Hit tests use `Rect::contains` on the same `constexpr Rect`s the drawing uses.
* Only the backdrop fills the margins (`area`); all panels, buttons and text sit inside 0..160 x 0..100.
* Line widths are in pixels, sizes in canvas units.
* Layout lives in `constexpr Rect` / `float` constants at the top of the file, in a `// ---- layout ----` block.
  Repeated items get a `xxxRect(index)` function.

Vertical bands used by every 160 x 100 screen:

| y | Part |
|---|---|
| 91.5–92 | title rule |
| 88 | title baseline |
| ~13–85 | panels |
| 9.5–17.5 | Back button (menu sub-screens); toasts at y 7–12 |
| 2.2 | key hint footer |

## Frame order

A screen's `Draw()`:

1. `glClear`, projection as above, `glDisable(GL_DEPTH_TEST)`.
2. Backdrop, title, panels (`DrawBackground`).
3. Contents: shapes and text, switching with `beginShapes()` / `beginText()`.
4. 3D models, if any (inventory): `glClear(GL_DEPTH_BUFFER_BIT)` first, so models never cut into the flat UI.
5. Footer (hint + toast).
6. Restore state (`glDisable(GL_BLEND)`, `glEnable(GL_TEXTURE_2D)`, `glEnable(GL_DEPTH_TEST)`, white colour),
   `Scenario::onFrameRendered()`, `glutSwapBuffers()`.

## Parts

### Backdrop

`textures.loadingBackground` (carved wall) over the whole `area`, tinted `{0.34, 0.27, 0.20}`, then a black
vignette: `ring(area.inset(22), 22, BLACK, 0, 0.85)`. The draft map uses 20.

### Title

`title` font, `textCentered(title, 80, 88, caption, GOLD)`. Either side a `GOLD_DIM` 2 px rule at y 91.5 (menu: 92),
from the title (text half width + 4) out to the reach. Big `GOLD` diamonds (1.1) at the rule ends, small ones (0.7)
next to the title. Reach from centre: 44 for a narrow panel (main menu, credits), 58 for wide panels, 72 on the riddle.

### Panel

`ui::panel(rect, 0.9f)`: brown gradient fill, 3 px `BRONZE` frame, 1 px `GOLD_DIM` inner line inset 1.1,
`GOLD` corner studs. The main container of a screen. Standard rects:

* Menu buttons: `{46, 19, 68, 64}`
* Save / Load slots: `{12, 22, 136, 62}`
* Options: `{12, 20, 136, 64}`
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

Raised tile, menu version `drawTile(rect, style, hovered, held)` in `menu.cpp`:

| State | Look |
|---|---|
| Idle | black drop shadow `(+0.5, -0.7)` alpha 0.4, gradient fill, 1 unit white top highlight alpha 0.08 |
| Hover | additive `GOLD` glow `ring(r, 2.2, GOLD, 0.4, 0)`, lighter fill, `GOLD_BRIGHT` 2.5 px frame, highlight 0.16 |
| Held | no shadow, sinks 0.4, darker fill, no highlight |
| Disabled | flat dark fill alpha 0.9, `BRONZE` frame alpha 0.6, no hover |

Styles:

* **Lapis**: primary action (New Game, Return to Game, active tab, inventory Equip / Upgrade / Drink).
  `LAPIS`→`LAPIS_DARK`, `GOLD` 2.5 px frame, `GOLD_DIM` inner line.
* **Stone**: all other actions and the save slots. `STONE_TOP`→`STONE_BOTTOM`, `BRONZE` 1.5 px frame.

One lapis button per group. Menu buttons are 56 x 9, 11.5 apart; Back is `{63, 9.5, 34, 8}`.
An action fires on mouse **up** over the same target it went down on (`pressed` / `hovered`).
The inventory has its own copies of the tile code (`DrawSlot`, `DrawButton`) with the same colours; on the papyrus a
disabled button is a faded tan tile with `INK_FADED` text.

### Icon well

Dark square on the left of a tile (`iconWell(tile, inset)`), `WELL` fill (lapis on a lapis tile), `GOLD_DIM` frame
(`GOLD` on hover). Holds a flat vector glyph from `drawIcon()` (Play, Save, Load, Gear, Ankh, Exit, Pyramid, Back)
or a number (save slots, lapis seal when the slot is used). Glyphs are shapes, not textures.

### Selection

Selected, not just hovered, items (inventory slots) get a breathing gold halo
(`ring`, alpha `0.35 + 0.25 * pulse`, additive) and a 3 px `GOLD` frame.

### Section heading

`heading` font in `GOLD` at the top left of a panel section, a 1 px `BRONZE` rule from after the text to the panel's
inner edge (inventory "Arms" / "Elixirs"). Table headers: `small` font in `LABEL`, rule under them, rows striped
with white alpha 0.035 (Options).

### Key caps

Options table: each key a dark gradient cap with a `GOLD_DIM` frame and a black shadow, key name in `small` `GOLD`.
Separators `/` and `,` are plain `LABEL_DIM` text.

### Footer and toast

* Key hint: `textCentered(small, 80, 2.2, hint, LABEL_DIM)`, keys and actions separated by 4 spaces
  (`"Click a slot to load    Esc: back"`).
* Toast: `body` font, `{1, 0.9, 0.6}`, above the footer, shown 2200 ms and faded out over the last 600 ms.

## Colours

Palette constants in `ui_draw.h`; use them, don't write new RGB values.

| Use | Constant |
|---|---|
| Titles, headings, primary text, lapis frame | `GOLD` |
| Hover frame and text | `GOLD_BRIGHT` (text `{1, 0.92, 0.65}`) |
| Rules, inner frame lines | `GOLD_DIM` |
| Outer frames, stone tile frame, section rules | `BRONZE` |
| Primary tile fill | `LAPIS` → `LAPIS_DARK` |
| Stone tile fill | `STONE_TOP` → `STONE_BOTTOM` |
| Panel fill | `PANEL_TOP` → `PANEL_BOTTOM` |
| Text on papyrus | `INK`, `INK_RED` (warnings, reward), `INK_GREEN`, `INK_FADED` (hints, disabled) |

`menu.cpp` adds `LABEL` `{0.86, 0.72, 0.47}` for body text on dark panels and `LABEL_DIM` `{0.55, 0.45, 0.30}` for
secondary text and footers.

## Fonts

All text uses `fonts/papyrus.png` (printable ASCII only, no accents: write "Skucas", not "Skučas").
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
