#ifndef FONT_H
#define FONT_H
#include <array>
#include <cstdint>
#include <memory>
#include "glyphs.h"

enum class TexFilter : std::uint8_t; // textures.h

struct FontSheet; // font.cpp: a sheet's texture and glyph edges, shared by every size loaded from it
class TrueTypeFace; // truetype.h
struct GlyphAtlas;

class Font {
  private:
	static constexpr int GLYPHS = FONT_GLYPHS; // printable ASCII from ' ', then the pad symbols (glyphs.h)
	std::shared_ptr<const FontSheet> sheet;
	float quad = 0; // the glyph quad's side (Load)
	float advance[GLYPHS] = {};
	float shift[GLYPHS] = {}; // a proportional glyph's quad starts this far left of the pen, so its ink starts there
	struct Cell {
		float u0, u1, v0, v1; // the glyph's cell in the sheet: left, right, bottom, top texture coordinate
		int turns; // quarter turns counter-clockwise (glyphs.h, sheetCell)
	};
	Cell cells[GLYPHS] = {};
	// A TrueType font (LoadTrueType) instead of a sheet: its face, its em in canvas units, and the atlas baked at the
	// pixel size it was last drawn at.
	std::shared_ptr<TrueTypeFace> face;
	float em = 0;
	std::shared_ptr<const GlyphAtlas> atlas;
	void drawTrueType(float x, float y, const char* text, const std::array<float, 4>& colour);

  public:
	// The sheet layout (tools/textures/font_sheet.py), in px of a glyph quad (a 51.2 px tenth of a 512 px sheet; a
	// bigger sheet scales it): the baseline sits this far above the quad bottom, the cap height reaches this far above
	// the baseline. Parchment, kalam and courier share it; impact (the HUD numbers) has a lower cap height (19 px).
	static constexpr float QUAD_PX = 51.2f;
	static constexpr float BASELINE_PX = 18.f;
	static constexpr float CAP_HEIGHT_PX = 24.f;

	Font();
	~Font();
	Font(const Font&) = delete;
	Font& operator=(const Font&) = delete;
	// Monospaced: every glyph advances size / 2 + spacing.
	// Proportional: each glyph advances its own inked width + spacing (measured from the texture).
	// Fonts of one file and filter share its texture while any of them lives. Flat (the default) suits text drawn
	// at about its sheet size or larger; Mipmapped keeps thin strokes whole when the text is drawn much smaller than
	// its sheet cells (the level editor's small text).
	void Load(const char filename[], float size = 12, float spacing = 0, bool proportional = false);
	void Load(const char filename[], float size, float spacing, bool proportional, TexFilter filter);
	// A TrueType font (plan sharp-text-ttf): the same quad `size` (its cap height and baseline as a sheet's), the
	// face's own advances and kerning plus `spacing`. Drawn from glyphs baked at the pixel size of the canvas it is
	// drawn in (re-baked when that changes), each glyph on whole pixels; the layout stays in canvas units.
	void LoadTrueType(const char filename[], float size, float spacing);
	// The text's glyph quads from the sheet (or the TrueType atlas) in one Batch draw (plan gl-core-ui), the quad
	// bottom of the first at x, y in gProj's canvas (the model view is not used), times `colour`; blend as the caller
	// set it.
	void draw(float x, float y, const char* text, const std::array<float, 4>& colour);
	[[nodiscard]] float TextWidth(const char* text) const;
	// draw() puts the quad bottom at y: the baseline is Baseline() above it, the cap top CapHeight() above that.
	[[nodiscard]] float Baseline() const { return quad * BASELINE_PX / QUAD_PX; }
	[[nodiscard]] float CapHeight() const { return quad * CAP_HEIGHT_PX / QUAD_PX; }
};

#endif
