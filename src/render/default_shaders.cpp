#include "default_shaders.h"
#include "shader.h"

#include <array>
#include <string>
#include "../core/logger.h"

namespace {
const char* const VERTEX_SRC = R"(
layout(location = 0) in vec4 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 6) in vec4 aColour;
layout(location = 7) in vec2 aUv;
uniform mat4 uMvp;
uniform mat4 uModelView;
uniform vec4 uTint;
uniform vec3 uLightDir;
uniform vec3 uLightAmbient;
uniform vec3 uLightDiffuse;
out vec4 vColour;
out vec2 vUv;
out vec3 vEye;
void main() {
	vEye = vec3(uModelView * aPos);
	vUv = aUv;
	vec4 colour = clamp(aColour * uTint, 0.0, 1.0); // as the fixed pipeline clamped glColor
#ifdef LIT
	// GL_NORMALIZE, GL_COLOR_MATERIAL (ambient and diffuse), no specular, no global ambient; clamped as the fixed
	// pipeline did, per vertex.
	vec3 n = normalize(transpose(inverse(mat3(uModelView))) * aNormal);
	float lambert = max(dot(n, normalize(uLightDir)), 0.0);
	colour.rgb = clamp(colour.rgb * (uLightAmbient + lambert * uLightDiffuse), 0.0, 1.0);
#endif
	vColour = colour;
	gl_Position = uMvp * aPos;
}
)";

const char* const FRAGMENT_SRC = R"(
uniform sampler2D uTex;
uniform float uAlphaRef;
uniform vec4 uFog; // colour, density per unit past uFogStart
uniform float uFogStart;
in vec4 vColour;
in vec2 vUv;
in vec3 vEye;
out vec4 fragColour;
void main() {
	vec4 colour = vColour;
#ifdef TEXTURE
	colour = texture(uTex, vUv) * colour;
#endif
#ifdef ALPHA_TEST
	if (colour.a <= uAlphaRef)
		discard;
#endif
	float haze = 1.0 - exp(-uFog.w * max(length(vEye) - uFogStart, 0.0));
	fragColour = vec4(mix(colour.rgb, uFog.rgb * colour.a, haze), colour.a);
}
)";

struct Program {
	GLuint id = 0;
	bool tried = false;
	GLint mvp = -1, modelView = -1, tint = -1, alphaRef = -1, fog = -1, fogStart = -1;
	GLint lightDir = -1, lightAmbient = -1, lightDiffuse = -1;
};

constexpr size_t KINDS = 4;
std::array<Program, KINDS> gPrograms;

const char* defines(DefaultShader::Kind kind) {
	switch (kind) {
	case DefaultShader::Kind::Colour:
		return "";
	case DefaultShader::Kind::Texture:
		return "#define TEXTURE\n";
	case DefaultShader::Kind::AlphaTest:
		return "#define TEXTURE\n#define ALPHA_TEST\n";
	case DefaultShader::Kind::Lit:
		return "#define TEXTURE\n#define LIT\n";
	}
	return "";
}

const char* name(DefaultShader::Kind kind) {
	switch (kind) {
	case DefaultShader::Kind::Colour:
		return "Default colour";
	case DefaultShader::Kind::Texture:
		return "Default texture";
	case DefaultShader::Kind::AlphaTest:
		return "Default alpha test";
	case DefaultShader::Kind::Lit:
		return "Default lit";
	}
	return "Default";
}

// Lazy, so it runs with a current GL context; tried once.
Program& program(DefaultShader::Kind kind) {
	Program& p = gPrograms[static_cast<size_t>(kind)];
	if (p.tried)
		return p;
	p.tried = true;
	const std::string head = std::string("#version 330 core\n") + defines(kind);
	const std::string vertex = head + VERTEX_SRC;
	const std::string fragment = head + FRAGMENT_SRC;
	p.id = linkProgram(name(kind), vertex.c_str(), fragment.c_str());
	if (p.id == 0)
		return p;
	p.mvp = glGetUniformLocation(p.id, "uMvp");
	p.modelView = glGetUniformLocation(p.id, "uModelView");
	p.tint = glGetUniformLocation(p.id, "uTint");
	p.alphaRef = glGetUniformLocation(p.id, "uAlphaRef");
	p.fog = glGetUniformLocation(p.id, "uFog");
	p.fogStart = glGetUniformLocation(p.id, "uFogStart");
	p.lightDir = glGetUniformLocation(p.id, "uLightDir");
	p.lightAmbient = glGetUniformLocation(p.id, "uLightAmbient");
	p.lightDiffuse = glGetUniformLocation(p.id, "uLightDiffuse");
	glUseProgram(p.id);
	glUniform1i(glGetUniformLocation(p.id, "uTex"), 0);
	glUseProgram(0);
	return p;
}
} // namespace

bool DefaultShader::use(const Style& style, const Mat4& projection, const Mat4& modelView) {
	const Program& p = program(style.kind);
	if (p.id == 0)
		return false;
	glUseProgram(p.id);
	const Mat4 mvp = projection * modelView;
	glUniformMatrix4fv(p.mvp, 1, GL_FALSE, mvp.data());
	glUniformMatrix4fv(p.modelView, 1, GL_FALSE, modelView.data());
	glUniform4fv(p.tint, 1, style.tint);
	glUniform1f(p.alphaRef, style.alphaRef);
	glUniform4f(p.fog, style.fog.r, style.fog.g, style.fog.b, style.fog.density);
	glUniform1f(p.fogStart, style.fog.start);
	const Light& l = style.light;
	glUniform3f(p.lightDir, l.direction.x, l.direction.y, l.direction.z);
	glUniform3f(p.lightAmbient, l.ambient.x, l.ambient.y, l.ambient.z);
	glUniform3f(p.lightDiffuse, l.diffuse.x, l.diffuse.y, l.diffuse.z);
	return true;
}
