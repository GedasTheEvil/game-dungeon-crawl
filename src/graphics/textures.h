#ifndef TEXTURES_H
#define TEXTURES_H
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

// True while a GL context is current: the GL objects below free themselves only then (closing the window destroys the
// context before the game's own cleanup runs).
[[nodiscard]] bool glContextCurrent();

// Owns its GL texture: freed with the object, moved but not copied (two copies would free it twice).
class Texture {
  private:
	TextureImage texture{};
	bool loaded;
	void free();

  public:
	Texture();
	~Texture();
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&& other) noexcept;
	// Changes the pixels before the upload: `components` (3 RGB, 4 RGBA) bytes per pixel, `pixels` of them.
	using PixelFilter = void (*)(unsigned char* data, int pixels, int components);
	// Replaces what it held.
	int LoadPNG(const char* filename, TexFilter filter = TexFilter::Mipmapped, PixelFilter change = nullptr);
	void Bind() const;
	void ClampToEdge() const; // no wrapping: for an image drawn once, not tiled
	[[nodiscard]] int ID() const;
};

#endif
