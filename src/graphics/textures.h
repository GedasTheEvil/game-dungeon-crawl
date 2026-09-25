#ifndef TEXTUROS
#define TEXTUROS

#include <GL/gl.h>

struct TextureImage // Create A Structure
{
	int width;	  // Image Width
	int height;	  // Image Height
	GLuint texID; // Texture ID Used To Select A Texture
};

class Textura {
  private:
	TextureImage texture{};
	bool loaded;

  public:
	Textura();
	int LoadPNG(const char* filename, bool mipmaps = false);
	void Bind();
	int ID();
};

#endif
