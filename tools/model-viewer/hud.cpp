#include "hud.h"
#include "../../src/render/batch.h"

namespace Hud {
void drawBar(float left, float bottom, float width, float height, float ratio, float red, float green, float blue) {
	static Batch batch;
	batch.style.kind = DefaultShader::Kind::Colour;
	batch.colour(1, 1, 1);
	batch.begin(Primitive::LineLoop);
	batch.vertex(left, bottom, 0);
	batch.vertex(left + width, bottom, 0);
	batch.vertex(left + width, bottom + height, 0);
	batch.vertex(left, bottom + height, 0);
	batch.end();

	batch.colour(red, green, blue);
	batch.begin(Primitive::Quads);
	batch.vertex(left, bottom, 0);
	batch.vertex(left + width * ratio, bottom, 0);
	batch.vertex(left + width * ratio, bottom + height, 0);
	batch.vertex(left, bottom + height, 0);
	batch.end();
}
} // namespace Hud
