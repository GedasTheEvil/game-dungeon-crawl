#ifndef DEFAULT_SHADERS_H
#define DEFAULT_SHADERS_H

#include <cstdint>
#include "gl_includes.h"
#include "mat4.h"
#include "wide_lines.h"

// The GL core path's default programs (plan gl-core-foundation), GLSL 330 core: what the fixed-function pipeline drew
// with glBegin, glColor, glTexCoord and GL_TEXTURE_2D. Batch draws with them; a VBO draw of a port can too.
namespace DefaultShader {

// The vertex attributes, the same in every default program and in Batch's vertex array. Colour and texture
// coordinate stay off 2 and 3, the lighting shader's for a model's next key frame.
constexpr GLuint POSITION = 0;
constexpr GLuint NORMAL = 1;
constexpr GLuint COLOUR = 6;
constexpr GLuint TEX_COORD = 7;

enum class Kind : std::uint8_t {
	Colour, // the vertex colour (glDisable(GL_TEXTURE_2D))
	Texture, // the vertex colour times the texture on unit 0 (GL_MODULATE)
	AlphaTest, // Texture, and a fragment at or below alphaRef is dropped (glAlphaFunc(GL_GREATER, alphaRef))
	Lit, // Texture, lit per vertex by one directional light (GL_LIGHT0 with GL_COLOR_MATERIAL, as the journal page)
};

// Distance haze, as the lighting shader's emissive draws take it: towards colour * alpha, so a premultiplied sprite's
// clear corners stay clear. density 0: none.
struct Fog {
	float r = 0.f, g = 0.f, b = 0.f;
	float density = 0.f; // per eye-space unit past start
	float start = 0.f;
};

// One directional light, eye space (glLightfv(GL_POSITION) with w 0, transformed by the model view when set).
struct Light {
	Vec3 direction{0.f, 0.f, 1.f}; // towards the light; normalised by the shader
	Vec3 ambient{0.f, 0.f, 0.f};
	Vec3 diffuse{1.f, 1.f, 1.f};
};

// What a draw looks like besides its vertices.
struct Style {
	Kind kind = Kind::Texture;
	float tint[4] = {1.f, 1.f, 1.f, 1.f}; // times every vertex colour (a glColor before a VBO draw)
	float alphaRef = 0.f; // AlphaTest
	Fog fog;
	Light light; // Lit
	WideLines::LineStyle line; // a line draw's width and smoothing, a model's outline (wide_lines.h)
};

// Binds the program of style.kind (and style.line's stage) and sets its uniforms: projection times model view (the
// matrices of gProj and gMv for Batch), the model view alone for the fog's distance and the light's normals. False
// (logged once) when the program does not build: the caller skips the draw.
bool use(const Style& style, const Mat4& projection, const Mat4& modelView);

} // namespace DefaultShader

#endif
