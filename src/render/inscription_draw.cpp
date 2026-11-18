#include "inscription_draw.h"
#include "lighting.h"

namespace {
void quad(float x0, float y0, float x1, float y1, float z, AtlasRect uv) {
	Batch& world = Lighting::batch();
	world.begin(Primitive::Quads);
	world.normal(0, 0, 1);
	world.texCoord(uv.u0, uv.v0);
	world.vertex(x0, y0, z);
	world.texCoord(uv.u1, uv.v0);
	world.vertex(x1, y0, z);
	world.texCoord(uv.u1, uv.v1);
	world.vertex(x1, y1, z);
	world.texCoord(uv.u0, uv.v1);
	world.vertex(x0, y1, z);
	world.end();
}
} // namespace

AtlasRect plateCell(InscriptionStyle style) { return atlasCell(static_cast<int>(style), PLATE_GRID); }

void drawInscription(
	float x,
	float y,
	float side,
	float z,
	InscriptionStyle style,
	const std::vector<std::string>& glyphs,
	const InscriptionArt& art,
	const float tint[3]
) {
	Batch& world =
		Lighting::batch(); // lit on the wall; outside Lighting::begin..end (the journal) texture times colour
	world.colour(tint[0], tint[1], tint[2]);
	art.plates.Bind();
	quad(x, y, x + side, y + side, z, plateCell(style));
	art.signs.Bind();
	for (const SignQuad& s : inscriptionLayout(style, glyphs, art.sheet)) {
		const float* paint = SIGN_PAINT[s.colour];
		world.colour(tint[0] * paint[0], tint[1] * paint[1], tint[2] * paint[2]);
		quad(x + s.x0 * side, y + s.y0 * side, x + s.x1 * side, y + s.y1 * side, z, art.sheet.Uv(s.cell));
	}
	world.colour(1, 1, 1);
}
