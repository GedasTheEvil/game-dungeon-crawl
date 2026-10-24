#ifndef INSCRIPTION_DRAW_H
#define INSCRIPTION_DRAW_H

// Drawing a composed inscription (plan composed-inscriptions): the plate of its style, then a quad per sign from the
// hieroglyph sheet, tinted with its paint (inscription_layout.h). The wall and the journal page both draw with it.

#include "../world/inscription_layout.h"
#include "textures.h"
#include <string>
#include <vector>

struct InscriptionArt {
	const Texture& plates; // inscription_plates.png
	const Texture& signs; // hieroglyphs.png
	const HieroglyphSheet& sheet; // its manifest
};

// Into the square of side `side` whose lower left corner is (x, y), at depth z, in the current frame (y up). The tint
// multiplies every part: white on the wall, the paper's colour in the journal. The caller sets the blending.
void drawInscription(
	float x,
	float y,
	float side,
	float z,
	InscriptionStyle style,
	const std::vector<std::string>& glyphs,
	const InscriptionArt& art,
	const float tint[3]
);

// The plate's cell of inscription_plates.png, for the glint over it.
[[nodiscard]] AtlasRect plateCell(InscriptionStyle style);

#endif
