#include "font.h"
#include <GL/gl.h>
#include <string.h>
#include <stdarg.h>
#include "../core/logger.h"
//=================================================================================================================
Font::Font() {}
//=================================================================================================================
void Font::print(int x, int y, const char* fmt, ...) // Where The Printing Happens
{
	if (fmt == nullptr) // If There's No Text
		return;			   // Do Nothing

	char text[256];		   // Holds Our String
	va_list ap;			   // Pointer To List Of Arguments
	va_start(ap, fmt);		// Parses The String For Variables
	vsprintf(text, fmt, ap); // And Converts Symbols To Actual Numbers
	va_end(ap);					// Results Are Stored In Text

	t.Bind();										   // Select Our Font Texture
	glPushMatrix();									   // Store The Modelview Matrix
	glLoadIdentity();								   // Reset The Modelview Matrix
	glTranslated(x, y, 1);							   // Position The Text (0,0 - Bottom Left)
	glListBase(base - 32);							   // Choose The Font Set
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text); // Draws The Display List Text
	glPopMatrix();									   // Restore The Old Projection Matrix
}
//=================================================================================================================
void Font::Load(const char filename[], float size, float spacing) // Build Our Font Display List
{
	base = glGenLists(95); // Creating 95 Display Lists
	if (!t.LoadTGA(filename))
		if (!t.LoadBMP(filename))
			LOG_ERRORF("graphics", "Could not load font texture: %s", filename);
	t.Bind();

	for (int loop = 0; loop < 95; loop++) // Loop Through All 95 Lists
	{
		float cx = static_cast<float>(loop % 10) / 10.0f; // X Position Of Current Character
		int row = loop / 10;                              // Integer row index 0..9
		float cy = static_cast<float>(row) / 10.0f;       // Y Position Of Current Character

		glNewList(base + loop, GL_COMPILE); // Start Building A List
		{
			glBegin(GL_QUADS); // Use A Quad For Each Character
			{
				glTexCoord2f(cx, 1.0f - cy - 0.1);
				glVertex2i(0, 0); // Texture / Vertex Coord (Bottom Left)
				glTexCoord2f(cx + 0.1, 1.0f - cy - 0.1);
				glVertex2i(size, 0); // Texutre / Vertex Coord (Bottom Right)
				glTexCoord2f(cx + 0.1, 1.0f - cy);
				glVertex2i(size, size); // Texture / Vertex Coord (Top Right)
				glTexCoord2f(cx, 1.0f - cy);
				glVertex2i(0, size); // Texture / Vertex Coord (Top Left)
			}
			glEnd();								// Done Building Our Quad (Character)
			glTranslated(size / 2 + spacing, 0, 0); // Move To The Right Of The Character
		}
		glEndList(); // Done Building The Display List
	} // Loop Until All Are Built
}
//=================================================================================================================
Font::~Font() {
	glDeleteLists(base, 95); // Delete All 95 Font Display Lists
	void* selfPtr = this;
	LOG_DEBUGF("graphics", "Deleting font %p", selfPtr);
}