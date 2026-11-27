#ifndef TRUETYPE_H
#define TRUETYPE_H
// TrueType fonts (plan sharp-text-ttf): a font file's layout metrics in ems, and its glyphs baked by FreeType at the
// pixel size the text is drawn at, so a glyph's texel is one screen pixel. No GL: Font uploads the bakes (font.cpp) and
// draws from them; the layout (advances, kerning) stays in canvas units and does not depend on the window size.
#include <array>
#include "glyphs.h"
#include <memory>
#include <vector>

// A glyph's place in a GlyphBitmaps image and where it sits against the pen, in pixels.
struct BakedGlyph {
	int x = 0, y = 0; // the bitmap's top-left texel in the image (row 0 is the image's first row)
	int width = 0, height = 0;
	int left = 0; // the bitmap's left edge right of the pen
	int top = 0; // its top row above the baseline
};

// The glyphs (glyphs.h: printable ASCII, the pad symbols) of a face rasterised at one pixel size, packed into one
// coverage image (0..255 a texel, rows top-down), with a free texel between glyphs so linear filtering never bleeds a
// neighbour in.
struct GlyphBitmaps {
	static constexpr int GLYPHS = FONT_GLYPHS; // glyphs.h
	int width = 0, height = 0;
	std::vector<unsigned char> coverage;
	std::array<BakedGlyph, GLYPHS> glyphs{};
};

class TrueTypeFace {
  public:
	static constexpr int GLYPHS = GlyphBitmaps::GLYPHS;
	// The face of a font file, opened once and shared while anything holds it; null when FreeType cannot read it.
	static std::shared_ptr<TrueTypeFace> Open(const char* filename);
	~TrueTypeFace();
	TrueTypeFace(const TrueTypeFace&) = delete;
	TrueTypeFace& operator=(const TrueTypeFace&) = delete;

	// In ems: the height of 'H', a glyph's advance, the kerning between two glyphs (0 .. GLYPHS-1).
	[[nodiscard]] float CapHeight() const { return capHeight; }
	[[nodiscard]] float Advance(int glyph) const { return advance[static_cast<size_t>(glyph)]; }
	[[nodiscard]] float Kerning(int previous, int glyph) const {
		return kerning.empty() ? 0.f : kerning[(static_cast<size_t>(previous) * GLYPHS) + static_cast<size_t>(glyph)];
	}
	// The glyphs rasterised at `emPx` pixels an em (hinted, light: vertical only, so the shapes stay true). No GL.
	[[nodiscard]] GlyphBitmaps Bake(float emPx) const;

  private:
	TrueTypeFace() = default;
	struct Impl;
	std::unique_ptr<Impl> impl;
	float capHeight = 0.7f;
	std::array<float, GLYPHS> advance{};
	std::vector<float> kerning; // GLYPHS x GLYPHS, empty when the face has no kerning
};

#endif
