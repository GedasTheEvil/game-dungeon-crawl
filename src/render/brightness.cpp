#include "brightness.h"
#include "gl/batch.h"
#include "gl/gl_state.h"
#include "gl/shader.h"

#include <array>
#include "gl/gl_includes.h"

namespace {
const char* const VERTEX_SRC = R"(
#version 330 core
layout(location = 0) in vec2 aPos; // drawScreenQuad's corners, clip space
out vec2 vUv;
void main() {
	vUv = aPos * 0.5 + 0.5;
	gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

// brightenedChannel: pow(v, 100 / percent).
const char* const FRAGMENT_SRC = R"(
#version 330 core
uniform sampler2D uFrame;
uniform vec2 uScale;
uniform float uExponent;
in vec2 vUv;
out vec4 fragColour;
void main() {
	vec3 colour = texture(uFrame, vUv * uScale).rgb;
	fragColour = vec4(pow(clamp(colour, 0.0, 1.0), vec3(uExponent)), 1.0);
}
)";

int gPercent = 100;
bool gTried = false;
GLuint gProgram = 0;
GLint gScale = -1, gExponent = -1;
GLuint gTexture = 0;
int gTexW = 0, gTexH = 0;

bool ensureProgram() {
	if (!gTried) {
		gTried = true;
		gProgram = linkProgram("Brightness", VERTEX_SRC, FRAGMENT_SRC);
		if (gProgram != 0) {
			gScale = glGetUniformLocation(gProgram, "uScale");
			gExponent = glGetUniformLocation(gProgram, "uExponent");
			glUseProgram(gProgram);
			glUniform1i(glGetUniformLocation(gProgram, "uFrame"), 0);
			glUseProgram(0);
		}
	}
	return gProgram != 0;
}

// A texture at least the frame's size; grows, never shrinks.
void ensureTexture(int width, int height) {
	if (gTexture != 0 && width <= gTexW && height <= gTexH)
		return;
	if (gTexture == 0)
		glGenTextures(1, &gTexture);
	gTexW = width;
	gTexH = height;
	glBindTexture(GL_TEXTURE_2D, gTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
}
} // namespace

void Brightness::set(int percent) { gPercent = percent; }

void Brightness::apply(int width, int height) {
	if (gPercent == 100 || width <= 0 || height <= 0 || !ensureProgram())
		return;
	const GlState saved = GlState::read();
	std::array<GLint, 4> viewport{};
	GLint activeTexture = GL_TEXTURE0;
	GLint boundTexture = 0;
	glGetIntegerv(GL_VIEWPORT, viewport.data());
	glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
	glActiveTexture(GL_TEXTURE0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
	glViewport(0, 0, width, height);
	ensureTexture(width, height);
	glBindTexture(GL_TEXTURE_2D, gTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);
	GlState pass = saved;
	pass.blend = false;
	pass.depthTest = false;
	pass.cull = false;
	pass.apply();
	glUseProgram(gProgram);
	glUniform2f(
		gScale,
		static_cast<float>(width) / static_cast<float>(gTexW),
		static_cast<float>(height) / static_cast<float>(gTexH)
	);
	glUniform1f(gExponent, 100.f / static_cast<float>(gPercent));
	drawScreenQuad();
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(boundTexture));
	glActiveTexture(static_cast<GLenum>(activeTexture));
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	saved.apply();
}
