#ifndef TEXTUROS
#define TEXTUROS

#include <GL/gl.h>
#include <cstdint>

struct TextureImage // Create A Structure
{
	int width;	  // Image Width
	int height;	  // Image Height
	GLuint texID; // Texture ID Used To Select A Texture
};

// Mipmapped = trilinear + anisotropic, for anything drawn in the 3D world.
// Flat = plain linear, for screen-space UI and glyph atlases whose cells would bleed into each other in the mips.
enum class TexFilter : std::uint8_t { Mipmapped, Flat };

class Textura {
  private:
	TextureImage texture{};
	bool loaded;

  public:
	Textura();
	int LoadPNG(const char* filename, TexFilter filter = TexFilter::Mipmapped);
	void Bind();
	int ID();
};

#endif
