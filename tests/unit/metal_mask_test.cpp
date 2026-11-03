// Metal shine (plan metallic-shading): the baked textures carry the metal mask in their alpha (255 matte, lower
// metal); a texture without metal stays opaque.
#include "../../external/doctest/doctest.h"
#include "../../src/core/paths.h"
#include "../../external/stb/stb_image.h"
#include <string>

namespace {
// The share of the texture's pixels with alpha below 255 (metal); 0 for a texture without alpha.
float metalShare(const std::string& name) {
	int width = 0;
	int height = 0;
	int channels = 0;
	const std::string file = MEDIA_DIR "textures/" + name + ".png";
	unsigned char* data = stbi_load(file.c_str(), &width, &height, &channels, 4);
	REQUIRE_MESSAGE(data != nullptr, file);
	int metal = 0;
	for (int i = 0; i < width * height; i++)
		metal += data[i * 4 + 3] < 255 ? 1 : 0;
	stbi_image_free(data);
	return static_cast<float>(metal) / static_cast<float>(width * height);
}
} // namespace

TEST_CASE("metal mask: the metal parts of the baked textures") {
	for (const char* name :
		 {"items/ring_band_copper",
		  "items/ring_band_silver",
		  "items/ring_band_gold",
		  "items/khopesh",
		  "items/amulet_strength",
		  "items/potion_flask_glass", // the glass shells' faint shine (plan potion-glass-vessels)
		  "items/potion_lotus_glass",
		  "items/potion_cobra_glass"}) {
		CAPTURE(name);
		CHECK(metalShare(name) > 0.02f);
	}
	CHECK(metalShare("items/sling") == 0.f);
	CHECK(metalShare("items/ring_gem") == 0.f);
	CHECK(metalShare("mechanisms/rock") == 0.f);
	CHECK(metalShare("monsters/crocodile") == 0.f);
}
