#ifndef UI_DRAW_H
#define UI_DRAW_H

// Flat 2D UI look shared by the game screens and the level editor: Egyptian palette, framed panels, tiles,
// two-pass text. Works in whatever ortho canvas is set up (y up); sizes are in canvas units, line widths in pixels.
// Conventions: docs/ui.md.

#include "ui_layout.h"
#include <cstdint>
#include <string>
#include <vector>

class Font;

namespace ui {

// A canvas `height` units tall and as wide as the window's aspect needs (square units, origin bottom left, y up), for
// the HUD parts and the draft map: sets the projection, leaves the modelview matrix current, returns the width.
// `depth`: the z range either side of 0.
float beginSquareCanvas(float height, int resX, int resY, float depth = 21.f);

// The four papyrus fonts of a UI screen; only the title's size differs between screens.
void loadScreenFonts(Font& title, Font& heading, Font& body, Font& small, float titleSize);

struct Color {
	float r, g, b;
};

// Egyptian palette: gold leaf, bronze, lapis lazuli, basalt, papyrus inks.
constexpr Color GOLD = {0.95f, 0.76f, 0.38f};
constexpr Color GOLD_DIM = {0.58f, 0.44f, 0.22f};
constexpr Color GOLD_BRIGHT = {1.f, 0.88f, 0.55f};
constexpr Color BRONZE = {0.42f, 0.30f, 0.15f};
constexpr Color LAPIS = {0.12f, 0.27f, 0.58f};
constexpr Color LAPIS_DARK = {0.05f, 0.12f, 0.30f};
constexpr Color STONE_TOP = {0.20f, 0.155f, 0.11f};
constexpr Color STONE_BOTTOM = {0.12f, 0.09f, 0.065f};
constexpr Color PANEL_TOP = {0.14f, 0.11f, 0.08f};
constexpr Color PANEL_BOTTOM = {0.08f, 0.06f, 0.045f};
constexpr Color BLACK = {0.f, 0.f, 0.f};
constexpr Color INK = {0.24f, 0.14f, 0.07f};
constexpr Color INK_RED = {0.62f, 0.17f, 0.08f};
constexpr Color INK_GREEN = {0.16f, 0.45f, 0.12f};
constexpr Color INK_FADED = {0.52f, 0.40f, 0.26f};
// The archaeologist's own pencil: the draft map's sketch, the journal's handwriting.
constexpr Color PENCIL = {0.22f, 0.21f, 0.20f};
// The journal's field notebook: blue ink for what is written, white paper, the cream elastic strap, the grey cloth.
constexpr Color INK_BLUE = {0.13f, 0.21f, 0.50f};
constexpr Color PAPER = {0.96f, 0.95f, 0.91f};
constexpr Color STRAP = {0.88f, 0.84f, 0.72f};
constexpr Color CLOTH = {0.85f, 0.85f, 0.82f};
// Tile states.
constexpr Color LAPIS_HOVER_TOP = {0.20f, 0.40f, 0.78f};
constexpr Color LAPIS_HOVER_BOTTOM = {0.09f, 0.20f, 0.46f};
constexpr Color LAPIS_HELD_BOTTOM = {0.03f, 0.07f, 0.18f};
constexpr Color STONE_HOVER_TOP = {0.33f, 0.25f, 0.15f};
constexpr Color STONE_HOVER_BOTTOM = {0.17f, 0.13f, 0.08f};
constexpr Color STONE_HELD_BOTTOM = {0.07f, 0.05f, 0.035f};
constexpr Color TEXT_HOVER = {1.f, 0.92f, 0.65f};

// ---- shapes (texturing off, see beginShapes) ----
void fillRect(const Rect& r, Color top, Color bottom, float alpha);
void strokeRect(const Rect& r, Color c, float alpha, float width);
void line(float x0, float y0, float x1, float y1, Color c, float alpha, float width);
void diamond(float x, float y, float size, Color c, float alpha);
void triangle(float x0, float y0, float x1, float y1, float x2, float y2, Color c, float alpha);
// Band around `inner`, `grow` wide, fading from alphaIn at the rect to alphaOut at the outer edge.
void ring(const Rect& inner, float grow, Color c, float alphaIn, float alphaOut);
void ellipse(float x, float y, float rx, float ry, Color c, float alpha);
// Framed panel: gradient fill, bronze outer frame, thin gold inner line and gold corner studs. frameAlpha fades
// the frame and studs too (the gameplay status box fades out).
void panel(const Rect& r, float alpha, float frameAlpha = 1.f);
// Gold studs on the four corners.
void cornerStuds(const Rect& r, float alpha = 1.f);
// Carved wall texture over the whole visible area, darkened towards the edges.
void backdrop(const Rect& area, int textureId);
// Screen title centred on `cx` at the top of the 100 high canvas, gold rules either side reaching `reach` from `cx`.
void titleBar(Font& font, float cx, const char* caption, float reach);

enum class TileStyle : std::uint8_t {
	Stone,			 // any action, save slots
	Lapis,			 // the primary action of a group
	Disabled,		 // on a dark panel
	PapyrusDisabled, // on a papyrus scroll
};
constexpr float TILE_SINK = 0.4f; // a held tile moves down this much
// Raised tile of the buttons and slots; returns the rect as drawn (it sinks while held). Disabled tiles ignore
// hovered / held.
Rect tile(Rect r, TileStyle style, bool hovered, bool held);
// Frame of an enabled tile, drawn by tile(); draw it again over content that covers the tile edges.
void tileFrame(const Rect& r, TileStyle style, bool hovered);
// Square on the left of a tile for its icon or number.
Rect iconWell(const Rect& tile, float inset);

// Textured quad; `u0..v1` pick the part of the texture. Leaves texturing off.
void texturedRect(const Rect& r, int textureId, Color tint, float u0 = 0, float v0 = 0, float u1 = 1, float v1 = 1);

// ---- text (texturing on, see beginText) ----
// The font sheets have no alpha, so the glyph brightness is used as coverage in two passes:
// first cut the glyph out of what is below, then add the colour into the hole.
void text(Font& font, float x, float y, const char* str, Color c, float alpha = 1.f);
void textCentered(Font& font, float cx, float y, const char* str, Color c, float alpha = 1.f);
// Words of `text` in lines no wider than `width`; a single longer word gets a line of its own.
std::vector<std::string> wrap(const Font& font, const std::string& text, float width);

// ---- screen tabs (their layout: ui_layout.h) ----
// hovered / held: a tab index or -1. The hovered tab's name shows on a label to the left of the strip.
void screenTabs(Font& small, ScreenTab open, int hovered, int held);

void beginShapes();
void beginText();

} // namespace ui

#endif
