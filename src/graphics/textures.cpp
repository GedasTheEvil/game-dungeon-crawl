#define GL_GLEXT_PROTOTYPES // glGenerateMipmap, exported by libGL on Linux
#include "textures.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <algorithm>
#include <cstring>
#include "../core/logger.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include "../../external/stb/stb_image.h"
#pragma GCC diagnostic pop

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
} // namespace

Textura::Textura() { loaded = false; }
//================================================================================================================================
int Textura::LoadPNG(const char* filename, TexFilter filter) {
	int channels = 0;
	if (stbi_info(filename, &texture.width, &texture.height, &channels) == 0) {
		LOG_WARNINGF("texture", "Cannot read %s: %s", filename, stbi_failure_reason());
		return 0;
	}

	// Grayscale PNGs are expanded to RGB; an alpha channel is kept.
	bool hasAlpha = channels == 2 || channels == 4;
	int components = hasAlpha ? 4 : 3;
	GLenum format = hasAlpha ? GL_RGBA : GL_RGB;

	// PNG rows run top-down, OpenGL expects the first row at the bottom (t = 0).
	stbi_set_flip_vertically_on_load(1);
	unsigned char* data = stbi_load(filename, &texture.width, &texture.height, &channels, components);
	if (data == nullptr) {
		LOG_ERRORF("texture", "Error loading PNG %s: %s", filename, stbi_failure_reason());
		return 0;
	}
	LOG_INFOF("texture", "%s: %dx%d, %d channels", filename, texture.width, texture.height, components);

	glGenTextures(1, &texture.texID);
	LOG_INFOF("texture", "Texture id=[%d]", texture.texID);

	glBindTexture(GL_TEXTURE_2D, texture.texID);
	bool mipmaps = filter == TexFilter::Mipmapped;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	if (mipmaps)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy());

	// stb_image packs rows tightly, so RGB rows need not be 4-byte aligned.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), texture.width, texture.height, 0, format,
				 GL_UNSIGNED_BYTE, data);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	if (mipmaps)
		glGenerateMipmap(GL_TEXTURE_2D); // keeps the real size; gluBuild2DMipmaps rescaled non-power-of-two images

	// OpenGL has its own copy on the GPU.
	stbi_image_free(data);

	loaded = true;

	return 1;
}
//----------------------------------------------------------------------------------
void Textura::Bind() { glBindTexture(GL_TEXTURE_2D, texture.texID); }
//----------------------------------------------------------------------------------
int Textura::ID() { return static_cast<int>(texture.texID); }
