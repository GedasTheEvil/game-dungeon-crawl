#include "font.h"
#include <GL/gl.h>
#include <string.h>
#include <stdarg.h>
#include "../core/logger.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#include "../../external/stb/stb_image.h"
#pragma GCC diagnostic pop

namespace {
constexpr int GRID = 10;				// the font sheet is 10 x 10 cells
constexpr unsigned char INK_LEVEL = 60; // brighter than this counts as glyph
constexpr float SPACE_WIDTH = 0.28f;	// of the glyph size, the space cell is empty

// Left and right inked column of every glyph cell, as fractions of the cell width.
// Returns false when the sheet cannot be read (the font stays monospaced).
bool measureGlyphs(const char* filename, int glyphs, float* left, float* right) {
	int width = 0;
	int height = 0;
	int channels = 0;
	stbi_set_flip_vertically_on_load(0);
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

//=================================================================================================================
Font::Font() {}
//=================================================================================================================
void Font::print(float x, float y, const char* fmt, ...) // Where The Printing Happens
{
	if (fmt == nullptr) // If There's No Text
		return;			// Do Nothing

	char text[256];			 // Holds Our String
	va_list ap;				 // Pointer To List Of Arguments
	va_start(ap, fmt);		 // Parses The String For Variables
	vsprintf(text, fmt, ap); // And Converts Symbols To Actual Numbers
	va_end(ap);				 // Results Are Stored In Text

	t.Bind();										   // Select Our Font Texture
	glPushMatrix();									   // Store The Modelview Matrix
	glLoadIdentity();								   // Reset The Modelview Matrix
	glTranslatef(x, y, 1);							   // Position The Text (0,0 - Bottom Left)
	glListBase(base - 32);							   // Choose The Font Set
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text); // Draws The Display List Text
	glPopMatrix();									   // Restore The Old Projection Matrix
}
//=================================================================================================================
float Font::TextWidth(const char* text) const {
	float width = 0.f;
	for (const char* c = text; *c != '\0'; c++) {
		int glyph = static_cast<unsigned char>(*c) - 32;
		if (glyph >= 0 && glyph < GLYPHS)
			width += advance[glyph];
	}
	return width;
}
//=================================================================================================================
void Font::Load(const char filename[], float size, float spacing, bool proportional) // Build Our Font Display List
{
	base = glGenLists(GLYPHS); // Creating 95 Display Lists
	if (!t.LoadPNG(filename))
		LOG_ERRORF("graphics", "Could not load font texture: %s", filename);
	t.Bind();

	float left[GLYPHS] = {};
	float right[GLYPHS] = {};
	if (proportional && !measureGlyphs(filename, GLYPHS, left, right)) {
		LOG_ERRORF("graphics", "Could not measure font glyphs: %s", filename);
		proportional = false;
	}

	for (int loop = 0; loop < GLYPHS; loop++) // Loop Through All 95 Lists
	{
		float cx = static_cast<float>(loop % 10) / 10.0f; // X Position Of Current Character
		int row = loop / 10;							  // Integer row index 0..9
		float cy = static_cast<float>(row) / 10.0f;		  // Y Position Of Current Character

		// Proportional glyphs are shifted left so the ink starts at the pen position.
		float shift = proportional ? -left[loop] * size : 0.f;
		// Monospaced fonts keep their historic whole-unit quads.
		float quad = proportional ? size : static_cast<float>(static_cast<int>(size));
		advance[loop] = size / 2 + spacing;
		if (proportional)
			advance[loop] =
				(right[loop] > left[loop] ? (right[loop] - left[loop]) * size : SPACE_WIDTH * size) + spacing;

		glNewList(base + loop, GL_COMPILE); // Start Building A List
		{
			glBegin(GL_QUADS); // Use A Quad For Each Character
			{
				glTexCoord2f(cx, 1.0f - cy - 0.1);
				glVertex2f(shift, 0); // Texture / Vertex Coord (Bottom Left)
				glTexCoord2f(cx + 0.1, 1.0f - cy - 0.1);
				glVertex2f(shift + quad, 0); // Texutre / Vertex Coord (Bottom Right)
				glTexCoord2f(cx + 0.1, 1.0f - cy);
				glVertex2f(shift + quad, quad); // Texture / Vertex Coord (Top Right)
				glTexCoord2f(cx, 1.0f - cy);
				glVertex2f(shift, quad); // Texture / Vertex Coord (Top Left)
			}
			glEnd();						   // Done Building Our Quad (Character)
			glTranslatef(advance[loop], 0, 0); // Move To The Right Of The Character
		}
		glEndList(); // Done Building The Display List
	} // Loop Until All Are Built
}
//=================================================================================================================
Font::~Font() {
	glDeleteLists(base, GLYPHS); // Delete All 95 Font Display Lists
	void* selfPtr = this;
	LOG_DEBUGF("graphics", "Deleting font %p", selfPtr);
}
