#include "textures.h"
#include <GL/gl.h>
#include <GL/glu.h>
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

Textura::Textura() { loaded = false; }
//================================================================================================================================
int Textura::LoadPNG(const char* filename, bool mipmaps) {
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

	// stb_image packs rows tightly, so RGB rows need not be 4-byte aligned.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (mipmaps)
		gluBuild2DMipmaps(GL_TEXTURE_2D, static_cast<GLint>(format), texture.width, texture.height, format,
						  GL_UNSIGNED_BYTE, data);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), texture.width, texture.height, 0, format,
					 GL_UNSIGNED_BYTE, data);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	// OpenGL has its own copy on the GPU.
	stbi_image_free(data);

	loaded = true;

	return 1;
}
//----------------------------------------------------------------------------------
void Textura::Bind() { glBindTexture(GL_TEXTURE_2D, texture.texID); }
//----------------------------------------------------------------------------------
int Textura::ID() { return static_cast<int>(texture.texID); }
