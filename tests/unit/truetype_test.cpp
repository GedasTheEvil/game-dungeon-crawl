// TrueType fonts (plan sharp-text-ttf): the layout stays in canvas units at every window size, and the glyphs bake at
// the drawn pixel size with the sheet fonts' cap height.
#include "../../external/doctest/doctest.h"
#include "../../src/render/font.h" // its constants only: Font links GL, the unit tests do not
#include "../../src/render/truetype.h"
#include "../../src/render/ui_draw.h"
#include <cmath>

namespace {
// Font::LoadTrueType's em for a quad `size`: H as tall as a sheet font's cap height.
float emOf(const TrueTypeFace& face, float size) {
	return size * Font::CAP_HEIGHT_PX / Font::QUAD_PX / face.CapHeight();
}

// Font::TextWidth: the advances plus `spacing` each, and the kerning.
float layoutWidth(const TrueTypeFace& face, float em, float spacing, const char* text) {
	float width = 0.f;
	int previous = -1;
	for (const char* c = text; *c != '\0'; c++) {
		const int glyph = *c - 32;
		if (previous >= 0)
			width += face.Kerning(previous, glyph) * em;
		previous = glyph;
		width += face.Advance(glyph) * em + spacing;
	}
	return width;
}

constexpr const char* SAMPLE = "Bronze khopesh: a curved sword of the old kingdom, +3 might.";

// Font::drawTrueType's pen: each glyph's left edge rounded to a whole pixel at `pxPerUnit`; the width from the first
// glyph's pen to the last one's end, back in canvas units.
float drawnWidth(const TrueTypeFace& face, float em, float spacing, float pxPerUnit, const char* text) {
	float pen = 0.f;
	float last = 0.f;
	int previous = -1;
	for (const char* c = text; *c != '\0'; c++) {
		const int glyph = *c - 32;
		if (previous >= 0)
			pen += face.Kerning(previous, glyph) * em;
		previous = glyph;
		last = std::round(pen * pxPerUnit) / pxPerUnit;
		pen += face.Advance(glyph) * em + spacing;
	}
	const int lastGlyph = text[std::char_traits<char>::length(text) - 1] - 32;
	return last + (face.Advance(lastGlyph) * em + spacing);
}
} // namespace

TEST_CASE("a TrueType font's layout does not change with the window size") {
	auto face = TrueTypeFace::Open(ui::DEJAVU_FONT);
	REQUIRE(face);
	const float em = emOf(*face, 3.6f);
	const float width = layoutWidth(*face, em, 0.1f, SAMPLE);
	CHECK(width > 50.f);
	CHECK(width < 90.f); // parchment's line at this size is a bit longer
	// 720p and 1440p on the 100 units tall canvas: 7.2 and 14.4 px a unit
	for (const float pxPerUnit : {7.2f, 10.8f, 14.4f}) {
		CAPTURE(pxPerUnit);
		CHECK(std::abs(drawnWidth(*face, em, 0.1f, pxPerUnit, SAMPLE) - width) <= 0.5f / pxPerUnit);
	}
}

TEST_CASE("a TrueType font bakes its glyphs at the pixel size, H at the cap height") {
	auto face = TrueTypeFace::Open(ui::DEJAVU_FONT);
	REQUIRE(face);
	const float capHeight = 3.6f * Font::CAP_HEIGHT_PX / Font::QUAD_PX;
	for (const float pxPerUnit : {7.2f, 14.4f}) {
		CAPTURE(pxPerUnit);
		const float emPx = emOf(*face, 3.6f) * pxPerUnit;
		const GlyphBitmaps baked = face->Bake(emPx);
		REQUIRE(baked.width > 0);
		REQUIRE(baked.coverage.size() == static_cast<size_t>(baked.width * baked.height));
		const BakedGlyph& h = baked.glyphs['H' - 32];
		// H stands on the baseline, its top within a pixel of the cap height (the hinting snaps it)
		CHECK(h.top - h.height == 0);
		CHECK(std::abs(static_cast<float>(h.top) - capHeight * pxPerUnit) <= 1.f);
		CHECK(baked.glyphs[0].width == 0); // the space has no ink
		// glyphs do not overlap and stay inside the image
		for (size_t i = 0; i < GlyphBitmaps::GLYPHS; i++) {
			const BakedGlyph& a = baked.glyphs[i];
			CHECK(a.x + a.width <= baked.width);
			CHECK(a.y + a.height <= baked.height);
			for (size_t j = i + 1; j < GlyphBitmaps::GLYPHS; j++) {
				const BakedGlyph& b = baked.glyphs[j];
				const bool apart = a.x + a.width < b.x || b.x + b.width < a.x || a.y + a.height < b.y ||
								   b.y + b.height < a.y || a.width == 0 || b.width == 0;
				CHECK(apart);
			}
		}
	}
}

TEST_CASE("a TrueType font has the face's kerning") {
	auto face = TrueTypeFace::Open(ui::DEJAVU_FONT);
	REQUIRE(face);
	CHECK(face->Kerning('A' - 32, 'V' - 32) < 0.f);
	const float em = emOf(*face, 3.6f);
	CHECK(layoutWidth(*face, em, 0.f, "AV") < layoutWidth(*face, em, 0.f, "A") + layoutWidth(*face, em, 0.f, "V"));
}
