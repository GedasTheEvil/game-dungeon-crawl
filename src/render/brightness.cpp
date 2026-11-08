#define GL_GLEXT_PROTOTYPES // GL 2.0 shader entry points, exported by libGL on Linux
#include "brightness.h"
#include "shader.h"

#include <GL/gl.h>
#include <GL/glext.h>

namespace {
const char* const VERTEX_SRC = R"(
#version 120
varying vec2 vUv;
void main() {
	vUv = gl_Vertex.xy * 0.5 + 0.5;
	gl_Position = gl_Vertex;
}
)";

// brightenedChannel: pow(v, 100 / percent).
const char* const FRAGMENT_SRC = R"(
#version 120
uniform sampler2D uFrame;
uniform vec2 uScale;
uniform float uExponent;
varying vec2 vUv;
void main() {
	vec3 colour = texture2D(uFrame, vUv * uScale).rgb;
	gl_FragColor = vec4(pow(clamp(colour, 0.0, 1.0), vec3(uExponent)), 1.0);
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
	glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);
	glViewport(0, 0, width, height);
	glActiveTexture(GL_TEXTURE0);
	ensureTexture(width, height);
	glBindTexture(GL_TEXTURE_2D, gTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glUseProgram(gProgram);
	glUniform2f(
		gScale,
		static_cast<float>(width) / static_cast<float>(gTexW),
		static_cast<float>(height) / static_cast<float>(gTexH)
	);
	glUniform1f(gExponent, 100.f / static_cast<float>(gPercent));
	glBegin(GL_QUADS); // clip space, the vertex shader ignores the matrices
	glVertex2f(-1.f, -1.f);
	glVertex2f(1.f, -1.f);
	glVertex2f(1.f, 1.f);
	glVertex2f(-1.f, 1.f);
	glEnd();
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glPopAttrib();
}
