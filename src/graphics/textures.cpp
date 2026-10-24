#define GL_GLEXT_PROTOTYPES // glGenerateMipmap, exported by libGL on Linux
#include "textures.h"
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <algorithm>
#include <cstring>
#include "../core/logger.h"
#include "../core/worker_pool.h"

#include "../../external/stb/stb_image.h"

namespace {
constexpr float MAX_ANISOTROPY = 8.f;

// The anisotropy the driver allows, capped at MAX_ANISOTROPY; 1 (off) without the extension.
float anisotropy() {
	static float level = -1.f;
	if (level < 0.f) {
		level = 1.f;
		const char* ext = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
		if (ext != nullptr && (strstr(ext, "GL_EXT_texture_filter_anisotropic") != nullptr ||
							   strstr(ext, "GL_ARB_texture_filter_anisotropic") != nullptr)) {
			float driverMax = 1.f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &driverMax);
			level = std::clamp(driverMax, 1.f, MAX_ANISOTROPY);
		}
		LOG_INFOF("texture", "Anisotropic filtering: %dx", static_cast<int>(level));
	}
	return level;
}

TextureBatch* openBatch = nullptr;

// Gives the texture `id` its pixels and filter.
void upload(GLuint id, const DecodedImage& image, TexFilter filter) {
	const GLenum format = image.components == 4 ? GL_RGBA : GL_RGB;
	glBindTexture(GL_TEXTURE_2D, id);
	bool mipmaps = filter == TexFilter::Mipmapped;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	if (mipmaps)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy());

	// stb_image packs rows tightly, so RGB rows need not be 4-byte aligned.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		static_cast<GLint>(format),
		image.width,
		image.height,
		0,
		format,
		GL_UNSIGNED_BYTE,
		image.pixels.get()
	);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	if (mipmaps)
		glGenerateMipmap(GL_TEXTURE_2D); // keeps the real size; gluBuild2DMipmaps rescaled non-power-of-two images
}
} // namespace

bool glContextCurrent() { return glXGetCurrentContext() != nullptr; }

Texture::Texture() { loaded = false; }

Texture::~Texture() { free(); }

Texture::Texture(Texture&& other) noexcept : texture(other.texture), loaded(other.loaded) { other.loaded = false; }

Texture& Texture::operator=(Texture&& other) noexcept {
	if (this != &other) {
		free();
		texture = other.texture;
		loaded = other.loaded;
		other.loaded = false;
	}
	return *this;
}

void Texture::free() {
	if (loaded && glContextCurrent())
		glDeleteTextures(1, &texture.texID);
	loaded = false;
}
//================================================================================================================================
DecodedImage Texture::Decode(const char* filename, PixelFilter change) {
	DecodedImage image;
	int channels = 0;
	if (stbi_info(filename, &image.width, &image.height, &channels) == 0) {
		image.error = std::string("Cannot read ") + filename + ": " + stbi_failure_reason();
		return image;
	}

	// Grayscale PNGs are expanded to RGB; an alpha channel is kept.
	image.components = channels == 2 || channels == 4 ? 4 : 3;

	// PNG rows run top-down, OpenGL expects the first row at the bottom (t = 0).
	stbi_set_flip_vertically_on_load_thread(1);
	image.pixels = {stbi_load(filename, &image.width, &image.height, &channels, image.components), stbi_image_free};
	if (image.pixels == nullptr) {
		image.error = std::string("Error loading PNG ") + filename + ": " + stbi_failure_reason();
		return image;
	}
	if (change != nullptr)
		change(image.pixels.get(), image.width * image.height, image.components);
	return image;
}
//================================================================================================================================
int Texture::LoadPNG(const char* filename, TexFilter filter, PixelFilter change) {
	if (TextureBatch* batch = TextureBatch::Open()) {
		free();
		glGenTextures(1, &texture.texID);
		glBindTexture(
			GL_TEXTURE_2D,
			texture.texID
		); // a texture from now on (glIsTexture), freed or not before the upload
		loaded = true;
		batch->Add(texture.texID, filename, filter, change);
		return 1;
	}

	const DecodedImage image = Decode(filename, change);
	if (image.pixels == nullptr) {
		LOG_WARNINGF("texture", "%s", image.error.c_str());
		return 0;
	}
	texture.width = image.width;
	texture.height = image.height;
	LOG_INFOF("texture", "%s: %dx%d, %d channels", filename, image.width, image.height, image.components);

	free();
	glGenTextures(1, &texture.texID);
	LOG_INFOF("texture", "Texture id=[%d]", texture.texID);
	upload(texture.texID, image, filter); // OpenGL keeps its own copy
	loaded = true;
	return 1;
}
//----------------------------------------------------------------------------------
void Texture::Bind() const { glBindTexture(GL_TEXTURE_2D, texture.texID); }
//----------------------------------------------------------------------------------
void Texture::ClampToEdge() const {
	Bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}
//----------------------------------------------------------------------------------
int Texture::ID() const { return static_cast<int>(texture.texID); }
//----------------------------------------------------------------------------------
TextureBatch::TextureBatch(WorkerPool& pool) : pool(pool) { openBatch = this; }
//----------------------------------------------------------------------------------
TextureBatch::~TextureBatch() {
	Finish();
	openBatch = nullptr;
}
//----------------------------------------------------------------------------------
TextureBatch* TextureBatch::Open() { return openBatch; }
//----------------------------------------------------------------------------------
void TextureBatch::Add(GLuint id, const char* filename, TexFilter filter, Texture::PixelFilter change) {
	std::string name = filename;
	auto image = pool.Run([name, change] { return Texture::Decode(name.c_str(), change); });
	pending.push_back({id, filter, std::move(name), std::move(image)});
	UploadReady();
}
//----------------------------------------------------------------------------------
void TextureBatch::upload(Pending& p) {
	const DecodedImage image = p.image.get();
	if (image.pixels == nullptr)
		LOG_WARNINGF("texture", "%s", image.error.c_str());
	else if (glIsTexture(p.id) == GL_FALSE) // freed before its upload: not made again
		LOG_INFOF("texture", "%s: freed before its upload", p.filename.c_str());
	else {
		LOG_INFOF(
			"texture",
			"%s: %dx%d, %d channels, id=[%d]",
			p.filename.c_str(),
			image.width,
			image.height,
			image.components,
			p.id
		);
		::upload(p.id, image, p.filter);
	}
	p.filename.clear();
}
//----------------------------------------------------------------------------------
void TextureBatch::UploadReady() {
	for (; uploaded < pending.size(); uploaded++) {
		if (pending[uploaded].image.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
			return;
		upload(pending[uploaded]);
	}
}
//----------------------------------------------------------------------------------
void TextureBatch::Finish() {
	for (; uploaded < pending.size(); uploaded++)
		upload(pending[uploaded]);
	pending.clear();
	uploaded = 0;
}
