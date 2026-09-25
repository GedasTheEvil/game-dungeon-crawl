#ifndef UI_DRAW_H
#define UI_DRAW_H

// Flat 2D UI look shared by the inventory and the level editor: Egyptian palette, framed panels, two-pass text.
// Works in whatever ortho canvas is set up (y up); sizes are in canvas units, line widths in pixels.

class Font;

namespace ui {

struct Rect {
	float x, y, w, h;
	[[nodiscard]] bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
	[[nodiscard]] float cx() const { return x + w / 2; }
	[[nodiscard]] float cy() const { return y + h / 2; }
	[[nodiscard]] Rect inset(float d) const { return {x + d, y + d, w - 2 * d, h - 2 * d}; }
};

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

// ---- shapes (texturing off, see beginShapes) ----
void fillRect(const Rect& r, Color top, Color bottom, float alpha);
void strokeRect(const Rect& r, Color c, float alpha, float width);
void line(float x0, float y0, float x1, float y1, Color c, float alpha, float width);
void diamond(float x, float y, float size, Color c, float alpha);
// Band around `inner`, `grow` wide, fading from alphaIn at the rect to alphaOut at the outer edge.
void ring(const Rect& inner, float grow, Color c, float alphaIn, float alphaOut);
void ellipse(float x, float y, float rx, float ry, Color c, float alpha);
// Framed panel: gradient fill, bronze outer frame, thin gold inner line and gold corner studs.
void panel(const Rect& r, float alpha);
// Gold studs on the four corners.
void cornerStuds(const Rect& r);
// Textured quad; `u0..v1` pick the part of the texture. Leaves texturing off.
void texturedRect(const Rect& r, int textureId, Color tint, float u0 = 0, float v0 = 0, float u1 = 1, float v1 = 1);

// ---- text (texturing on, see beginText) ----
// The font sheets have no alpha, so the glyph brightness is used as coverage in two passes:
// first cut the glyph out of what is below, then add the colour into the hole.
void text(Font& font, float x, float y, const char* str, Color c, float alpha = 1.f);
void textCentered(Font& font, float cx, float y, const char* str, Color c, float alpha = 1.f);

void beginShapes();
void beginText();

} // namespace ui

#endif
