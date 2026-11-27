#include "truetype.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include "../core/logger.h"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace {
constexpr int ATLAS_WIDTH = 512;
constexpr int GUTTER = 1; // free texels between glyphs
constexpr int FIRST_CHAR = 32;

FT_Library library() {
	static FT_Library lib = nullptr;
	static bool tried = false;
	if (!tried) {
		tried = true;
		if (FT_Init_FreeType(&lib) != 0) {
			LOG_ERRORF("graphics", "%s", "Could not start FreeType");
			lib = nullptr;
		}
	}
	return lib;
}

// A 26.6 fixed-point value in pixels.
float fromFixed(FT_Pos value) { return static_cast<float>(value) / 64.f; }
} // namespace

struct TrueTypeFace::Impl {
	FT_Face face = nullptr;
	std::array<FT_UInt, GLYPHS> index{}; // the face's glyph index of each character
};

std::shared_ptr<TrueTypeFace> TrueTypeFace::Open(const char* filename) {
	static std::map<std::string, std::weak_ptr<TrueTypeFace>> faces;
	std::weak_ptr<TrueTypeFace>& cached = faces[filename];
	if (auto shared = cached.lock())
		return shared;
	FT_Library lib = library();
	FT_Face face = nullptr;
	if (lib == nullptr || FT_New_Face(lib, filename, 0, &face) != 0) {
		LOG_ERRORF("graphics", "Could not load TrueType font: %s", filename);
		return nullptr;
	}
	std::shared_ptr<TrueTypeFace> result(new TrueTypeFace());
	result->impl = std::make_unique<Impl>();
	result->impl->face = face;
	const auto em = static_cast<float>(face->units_per_EM);
	for (int glyph = 0; glyph < GLYPHS; glyph++) {
		const FT_UInt index = FT_Get_Char_Index(face, glyphCodePoint(glyph));
		result->impl->index[static_cast<size_t>(glyph)] = index;
		if (FT_Load_Glyph(face, index, FT_LOAD_NO_SCALE) == 0)
			result->advance[static_cast<size_t>(glyph)] = static_cast<float>(face->glyph->metrics.horiAdvance) / em;
	}
	if (FT_Load_Glyph(face, result->impl->index['H' - FIRST_CHAR], FT_LOAD_NO_SCALE) == 0)
		result->capHeight = static_cast<float>(face->glyph->metrics.horiBearingY) / em;
	if (FT_HAS_KERNING(face)) {
		result->kerning.assign(static_cast<size_t>(GLYPHS) * GLYPHS, 0.f);
		for (int a = 0; a < GLYPHS; a++)
			for (int b = 0; b < GLYPHS; b++) {
				FT_Vector kern{};
				FT_Get_Kerning(
					face,
					result->impl->index[static_cast<size_t>(a)],
					result->impl->index[static_cast<size_t>(b)],
					FT_KERNING_UNSCALED,
					&kern
				);
				result->kerning[(static_cast<size_t>(a) * GLYPHS) + static_cast<size_t>(b)] = static_cast<float>(
																								  kern.x
																							  ) /
																							  em;
			}
	}
	cached = result;
	return result;
}

TrueTypeFace::~TrueTypeFace() {
	if (impl && impl->face != nullptr)
		FT_Done_Face(impl->face);
}

GlyphBitmaps TrueTypeFace::Bake(float emPx) const {
	GlyphBitmaps out;
	FT_Face face = impl->face;
	if (FT_Set_Char_Size(face, 0, static_cast<FT_F26Dot6>(std::lround(emPx * 64.f)), 72, 72) != 0)
		return out;
	// Rasterise every glyph first, then pack them on shelves of the line height.
	struct Bitmap {
		BakedGlyph place;
		std::vector<unsigned char> pixels;
	};
	std::array<Bitmap, GLYPHS> bitmaps;
	for (size_t glyph = 0; glyph < GLYPHS; glyph++) {
		if (FT_Load_Glyph(face, impl->index[glyph], FT_LOAD_TARGET_LIGHT) != 0 ||
			FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LIGHT) != 0)
			continue;
		const FT_Bitmap& bitmap = face->glyph->bitmap;
		Bitmap& b = bitmaps[glyph];
		b.place.width = static_cast<int>(bitmap.width);
		b.place.height = static_cast<int>(bitmap.rows);
		b.place.left = face->glyph->bitmap_left;
		b.place.top = face->glyph->bitmap_top;
		b.pixels.resize(static_cast<size_t>(b.place.width) * static_cast<size_t>(b.place.height));
		for (int row = 0; row < b.place.height; row++)
			std::copy_n(
				bitmap.buffer + static_cast<ptrdiff_t>(row) * bitmap.pitch,
				b.place.width,
				b.pixels.begin() + static_cast<ptrdiff_t>(row) * b.place.width
			);
	}
	const int lineHeight = static_cast<int>(std::ceil(fromFixed(face->size->metrics.height))) + GUTTER;
	int x = GUTTER;
	int y = GUTTER;
	int shelf = 0;
	for (Bitmap& b : bitmaps) {
		if (x + b.place.width + GUTTER > ATLAS_WIDTH) {
			x = GUTTER;
			y += shelf + GUTTER;
			shelf = 0;
		}
		b.place.x = x;
		b.place.y = y;
		x += b.place.width + GUTTER;
		shelf = std::max({shelf, b.place.height, lineHeight - GUTTER});
	}
	out.width = ATLAS_WIDTH;
	out.height = y + shelf + GUTTER;
	out.coverage.assign(static_cast<size_t>(out.width) * static_cast<size_t>(out.height), 0);
	for (size_t glyph = 0; glyph < GLYPHS; glyph++) {
		const Bitmap& b = bitmaps[glyph];
		out.glyphs[glyph] = b.place;
		for (int row = 0; row < b.place.height; row++)
			std::copy_n(
				b.pixels.begin() + static_cast<ptrdiff_t>(row) * b.place.width,
				b.place.width,
				out.coverage.begin() + static_cast<ptrdiff_t>(b.place.y + row) * out.width + b.place.x
			);
	}
	return out;
}
