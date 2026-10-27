// Font metrics (plan text-vertical-centre): Font's baseline and cap height match the inked "H" of the font sheets.
#include "../../external/doctest/doctest.h"
#include "../../src/graphics/font.h"
#include "../../src/core/paths.h"
#include "../../external/stb/stb_image.h"
#include <algorithm>
#include <cmath>

namespace {
struct Ink {
	float bottom; // above the quad bottom, in px of a glyph quad
	float top;
};

// The inked rows of `glyph`'s cell, as Font::Load maps the cell to its quad (a tenth of the sheet).
Ink inkOf(const char* file, char glyph) {
	int width = 0;
	int height = 0;
	int channels = 0;
	stbi_set_flip_vertically_on_load_thread(0);
	unsigned char* data = stbi_load(file, &width, &height, &channels, 1);
	REQUIRE(data != nullptr);
	const int cell = glyph - 32;
	const float cellH = static_cast<float>(height) / 10.f;
	const int x0 = (cell % 10) * width / 10;
	const int x1 = (cell % 10 + 1) * width / 10;
	const float quadTop = static_cast<float>(cell / 10) * cellH;
	int first = height;
	int last = -1;
	for (int y = static_cast<int>(quadTop); y < static_cast<int>(quadTop + cellH); y++)
		for (int x = x0; x < x1; x++)
			if (data[y * width + x] > 60) {
				first = std::min(first, y);
				last = y;
			}
	stbi_image_free(data);
	REQUIRE(last >= 0);
	const float scale = Font::QUAD_PX / cellH;
	const float quadBottom = quadTop + cellH;
	return {(quadBottom - static_cast<float>(last + 1)) * scale, (quadBottom - static_cast<float>(first)) * scale};
}
} // namespace

TEST_CASE("the font sheets' H sits on the baseline and reaches the cap height") {
	// impact.png (the HUD numbers) has a lower cap height, font.h
	for (const char* file :
		 {MEDIA_DIR "textures/fonts/papyrus.png",
		  MEDIA_DIR "textures/fonts/kalam.png",
		  MEDIA_DIR "textures/fonts/courier.png"}) {
		CAPTURE(file);
		const Ink h = inkOf(file, 'H');
		CHECK(std::abs(h.bottom - Font::BASELINE_PX) <= 1.f); // within 1 px
		CHECK(std::abs(h.top - h.bottom - Font::CAP_HEIGHT_PX) <= 1.f);
	}
}
