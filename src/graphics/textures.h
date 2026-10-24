#ifndef TEXTURES_H
#define TEXTURES_H
#include <GL/gl.h>
#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <vector>

class WorkerPool;

struct TextureImage // Create A Structure
{
	int width; // Image Width
	int height; // Image Height
	GLuint texID; // Texture ID Used To Select A Texture
};

// Mipmapped = trilinear + anisotropic, for anything drawn in the 3D world.
// Flat = plain linear, for screen-space UI and glyph atlases whose cells would bleed into each other in the mips.
enum class TexFilter : std::uint8_t { Mipmapped, Flat };

// The pixels of a PNG, decoded without GL (any thread): rows bottom-up, `components` (3 RGB, 4 RGBA) bytes per pixel.
struct DecodedImage {
	std::unique_ptr<unsigned char, void (*)(void*)> pixels{nullptr, nullptr}; // null: error says why
	int width = 0, height = 0, components = 0;
	std::string error;
};

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
	// Replaces what it held. While a TextureBatch is open, the pixels come later: 1 then only says the GL texture is
	// reserved.
	int LoadPNG(const char* filename, TexFilter filter = TexFilter::Mipmapped, PixelFilter change = nullptr);
	static DecodedImage Decode(const char* filename, PixelFilter change = nullptr);
	void Bind() const;
	void ClampToEdge() const; // no wrapping: for an image drawn once, not tiled
	[[nodiscard]] int ID() const;
};

// Plan faster-loading: while a batch is open (on the main thread), Texture::LoadPNG reserves the GL texture at once and
// decodes the PNG on the pool's threads. The decoded images are uploaded in the order they were asked for: those ready
// at each LoadPNG, the rest by Finish (or when the batch closes). The texture's ID, ClampToEdge and Bind work before
// the upload.
class TextureBatch {
  private:
	struct Pending {
		GLuint id;
		TexFilter filter;
		std::string filename;
		std::future<DecodedImage> image;
	};
	WorkerPool& pool;
	std::vector<Pending> pending;
	size_t uploaded = 0; // pending[0, uploaded) are done
	void upload(Pending& p);

  public:
	explicit TextureBatch(WorkerPool& pool);
	~TextureBatch();
	TextureBatch(const TextureBatch&) = delete;
	TextureBatch& operator=(const TextureBatch&) = delete;
	static TextureBatch* Open(); // null if none
	void Add(GLuint id, const char* filename, TexFilter filter, Texture::PixelFilter change);
	void UploadReady(); // in order, up to the first one still decoding
	void Finish(); // waits for every one
};

#endif
