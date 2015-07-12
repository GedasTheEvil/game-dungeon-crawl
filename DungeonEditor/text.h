// #include "text.h"
void glPrint(GLint x, GLint y, const char *string, ...);
void BuildFont();
void Deinitialize (void);

void glPrint(GLint x, GLint y, const char *string, ...)	// Where The Printing Happens
{
	char		text[256];				// Holds Our String
	va_list		ap;					// Pointer To List Of Arguments
	if (string == NULL)					// If There's No Text
		return;						// Do Nothing

	va_start(ap, string);					// Parses The String For Variables
	    vsprintf(text, string, ap);				// And Converts Symbols To Actual Numbers
	va_end(ap);						// Results Are Stored In Text

	glBindTexture(GL_TEXTURE_2D, texttex.texID);			// Select Our Font Texture
	glPushMatrix();						// Store The Modelview Matrix
	glLoadIdentity();					// Reset The Modelview Matrix
	glTranslated(x,y,1);					// Position The Text (0,0 - Bottom Left)
	glListBase(base-32);					// Choose The Font Set
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
	glPopMatrix();						// Restore The Old Projection Matrix
}

void BuildFont()				// Build Our Font Display List
{
	base=glGenLists(95);					// Creating 95 Display Lists
	glBindTexture(GL_TEXTURE_2D,texttex.texID);	// Bind Our Font Texture
	int loop;
	for (loop=0; loop<95; loop++)			// Loop Through All 95 Lists
	{
		float cx=(float)(loop%16)/16.0f;			// X Position Of Current Character
		float cy=(float)(loop/16)/8.0f;			// Y Position Of Current Character

		glNewList(base+loop,GL_COMPILE);		// Start Building A List
		{
			glBegin(GL_QUADS);			// Use A Quad For Each Character
			{
				glTexCoord2f(cx,         1.0f-cy-0.120f); glVertex2i(0,0);	// Texture / Vertex Coord (Bottom Left)
				glTexCoord2f(cx+0.0625f, 1.0f-cy-0.120f); glVertex2i(12,0);	// Texutre / Vertex Coord (Bottom Right)
				glTexCoord2f(cx+0.0625f, 1.0f-cy);		  glVertex2i(12,12);// Texture / Vertex Coord (Top Right)
				glTexCoord2f(cx,         1.0f-cy);		  glVertex2i(0,12);	// Texture / Vertex Coord (Top Left)
			}
			glEnd();				// Done Building Our Quad (Character)
			glTranslated(7,0,0);			// Move To The Right Of The Character
		}
		glEndList();					// Done Building The Display List
	}							// Loop Until All 256 Are Built
// 	Minimap();
}

void Deinitialize (void)				// Any User DeInitialization Goes Here
{
// 	int nCo/*nt;*/
	
	glDeleteLists(base,95);				// Delete All 95 Font Display Lists
	
	free(texttex.imageData);

}


