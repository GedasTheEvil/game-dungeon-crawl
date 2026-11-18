#include "font.h"
#include "batch.h"
#include "gl_legacy.h"
#include "matrix_stack.h"
#include "textures.h"
#include <string.h>
#include <stdarg.h>
#include <map>
#include <string>
#include <utility>
#include "../core/logger.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#include "../../external/stb/stb_image.h"
#pragma GCC diagnostic pop

namespace {
constexpr int GRID = 10; // the font sheet is 10 x 10 cells
constexpr unsigned char INK_LEVEL = 60; // brighter than this counts as glyph
constexpr float SPACE_WIDTH = 0.28f; // of the glyph size, the space cell is empty

constexpr int SHEET_GLYPHS = 95;

// Left and right inked column of every glyph cell, as fractions of the cell width.
// Returns false when the sheet cannot be read (the font stays monospaced).
bool measureGlyphs(const char* filename, int glyphs, float* left, float* right) {
	int width = 0;
	int height = 0;
	int channels = 0;
	stbi_set_flip_vertically_on_load_thread(0); // per thread, as Texture::Decode sets it
	unsigned char* data = stbi_load(filename, &width, &height, &channels, 1);
	if (data == nullptr)
		return false;

	int cellW = width / GRID;
	int cellH = height / GRID;
	for (int glyph = 0; glyph < glyphs; glyph++) {
		int x0 = (glyph % GRID) * cellW;
		int y0 = (glyph / GRID) * cellH;
		int first = cellW;
		int last = -1;
		for (int x = 0; x < cellW; x++)
			for (int y = 0; y < cellH; y++)
				if (data[(y0 + y) * width + x0 + x] > INK_LEVEL) {
					first = first < x ? first : x;
					last = x;
					break;
				}
		left[glyph] = last < 0 ? 0.f : static_cast<float>(first) / static_cast<float>(cellW);
		right[glyph] = last < 0 ? 0.f : static_cast<float>(last + 1) / static_cast<float>(cellW);
	}
	stbi_image_free(data);
	return true;
}
} // namespace

struct FontSheet {
	Texture texture;
	bool measured = false; // left / right hold the glyph edges (measureGlyphs)
	float left[SHEET_GLYPHS] = {};
	float right[SHEET_GLYPHS] = {};
};

namespace {
// The sheet of a font file and filter, loaded on first use and kept while a font holds it.
std::shared_ptr<const FontSheet> loadSheet(const char* filename, TexFilter filter) {
	static std::map<std::pair<std::string, TexFilter>, std::weak_ptr<const FontSheet>> sheets;
	std::weak_ptr<const FontSheet>& cached = sheets[{filename, filter}];
	if (auto shared = cached.lock())
		return shared;
	auto sheet = std::make_shared<FontSheet>();
	if (!sheet->texture.LoadPNG(filename, filter))
		LOG_ERRORF("graphics", "Could not load font texture: %s", filename);
	sheet->measured = measureGlyphs(filename, SHEET_GLYPHS, sheet->left, sheet->right);
	cached = sheet;
	return sheet;
}
} // namespace

Font::Font() {}

void Font::draw(float x, float y, const char* text, const std::array<float, 4>& colour) {
	if (text == nullptr || *text == '\0' || !sheet)
		return;
	static Batch batch;
	batch.style.kind = DefaultShader::Kind::Texture;
	sheet->texture.Bind();
	// The legacy font was a display list a glyph, called with the model view loaded with glTranslatef(x, y, 1) and
	// moved by glTranslatef(advance, 0, 0) after each glyph. Here each corner is transformed on the CPU by the
	// projection times that glyph's matrix (Mat4 rounds as the GL did: the render snapshots match), and the whole text
	// is drawn in one batch under identity matrices, which keep the corners as they are.
	const Mat4 projection = gProj.top();
	float pen = x;
	batch.colour(colour[0], colour[1], colour[2], colour[3]);
	batch.begin(Primitive::Triangles);
	for (const char* c = text; *c != '\0'; c++) {
		const int glyph = static_cast<unsigned char>(*c) - 32;
		if (glyph < 0 || glyph >= GLYPHS) // no display list: glCallLists drew and moved nothing
			continue;
		const Mat4 mvp = projection * Mat4::translation(pen, y, 1.f);
		const float left = shift[glyph];
		const Cell& cell = cells[glyph];
		const float corners[4][4] = {
			{cell.u0, cell.v0, left, 0.f},
			{cell.u1, cell.v0, left + quad, 0.f},
			{cell.u1, cell.v1, left + quad, quad},
			{cell.u0, cell.v1, left, quad},
		};
		// A display list's quad a, b, c, d as Mesa compiled it: a, b, d and b, c, d (not the a, b, c / a, c, d of an
		// immediate GL_QUADS, Batch's Quads), so the text rasterises as before.
		for (const int k : {0, 1, 3, 1, 2, 3}) {
			const float* q = corners[k];
			const Vec4 clip = mvp * Vec4{q[2], q[3], 0.f, 1.f};
			batch.texCoord(q[0], q[1]);
			batch.vertex(clip.x, clip.y, clip.z);
		}
		pen = advance[glyph] + pen;
	}
	gProj.push();
	gProj.loadIdentity();
	gMv.push();
	gMv.loadIdentity();
	batch.end();
	gMv.pop();
	gProj.pop();
}

void Font::print(float x, float y, const char* fmt, ...) {
	if (fmt == nullptr)
		return;
	char text[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(text, sizeof(text), fmt, ap); // cut at 255 chars
	va_end(ap);
	GlLegacy::adoptMatrices();
	std::array<float, 4> colour{};
	GlLegacy::currentColour(colour.data());
	draw(x, y, text, colour);
}

float Font::TextWidth(const char* text) const {
	float width = 0.f;
	for (const char* c = text; *c != '\0'; c++) {
		int glyph = static_cast<unsigned char>(*c) - 32;
		if (glyph >= 0 && glyph < GLYPHS)
			width += advance[glyph];
	}
	return width;
}
void Font::Load(const char filename[], float size, float spacing, bool proportional) {
	Load(filename, size, spacing, proportional, TexFilter::Flat);
}
void Font::Load(const char filename[], float size, float spacing, bool proportional, TexFilter filter) {
	static_assert(GLYPHS == SHEET_GLYPHS);
	sheet = loadSheet(filename, filter);
	const float* left = sheet->left;
	const float* right = sheet->right;
	if (proportional && !sheet->measured) {
		LOG_ERRORF("graphics", "Could not measure font glyphs: %s", filename);
		proportional = false;
	}
	// Monospaced fonts keep their historic whole-unit quads.
	quad = proportional ? size : static_cast<float>(static_cast<int>(size));
	for (int glyph = 0; glyph < GLYPHS; glyph++) {
		const float cx = static_cast<float>(glyph % GRID) / 10.0f; // the cell's left edge in the sheet
		const int row = glyph / GRID;
		const float cy = static_cast<float>(row) / 10.0f; // its top edge, from the sheet's top
		cells[glyph] = {cx, cx + 0.1f, 1.0f - cy - 0.1f, 1.0f - cy};
		shift[glyph] = proportional ? -left[glyph] * size : 0.f;
		advance[glyph] = size / 2 + spacing;
		if (proportional)
			advance[glyph] = (right[glyph] > left[glyph] ? (right[glyph] - left[glyph]) * size : SPACE_WIDTH * size) +
							 spacing;
	}
}
Font::~Font() = default;
