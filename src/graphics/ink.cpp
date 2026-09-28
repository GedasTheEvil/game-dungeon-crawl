#define GL_GLEXT_PROTOTYPES // GL 2.0 shader and 3.0 framebuffer entry points, exported by libGL on Linux
#include "ink.h"

#include <GL/gl.h>
#include <GL/glext.h>
#include "../core/logger.h"
#include "../state/game_state.h"

namespace {
const char* const VERTEX_SRC = R"(
#version 120
varying vec2 vUv;
void main() {
	vUv = gl_Vertex.xy * 0.5 + 0.5;
	gl_Position = gl_Vertex;
}
)";

// Depth is turned into w = 1 / eye distance, which is affine across any flat surface on screen: its second
// difference is zero on a plane and jumps on a crease, whatever the viewing angle. Silhouettes are a big
// relative distance step to a neighbour, drawn only on the nearer side so the line hugs the front object.
const char* const FRAGMENT_SRC = R"(
#version 120
uniform sampler2D uColor;
uniform sampler2D uDepth;
uniform vec2 uPixel; // one pixel in uv
uniform float uWidth; // line half width in pixels
uniform float uNear;
uniform float uFar;
varying vec2 vUv;
float w(vec2 uv) {
	float d = texture2D(uDepth, uv).r;
	return (uFar - d * (uFar - uNear)) / (uNear * uFar);
}
float edgeAt(vec2 uv) {
	vec2 dirs[4];
	dirs[0] = vec2(1.0, 0.0);
	dirs[1] = vec2(0.0, 1.0);
	dirs[2] = vec2(0.7, 0.7);
	dirs[3] = vec2(0.7, -0.7);
	float wc = w(uv);
	float silhouette = 0.0;
	float nearer = 0.0; // a neighbour in front: this pixel is the far side of someone else's outline
	float crease = 0.0;
	for (int i = 0; i < 4; i++) {
		vec2 o = dirs[i] * uPixel * uWidth;
		float wa = w(uv + o);
		float wb = w(uv - o);
		// w shrinks with distance: wc / wn - 1 is how much further the neighbour is, relative to this pixel
		silhouette = max(silhouette, max(wc / wa, wc / wb) - 1.0);
		nearer = max(nearer, max(wa / wc, wb / wc) - 1.0);
		crease = max(crease, abs(wa + wb - 2.0 * wc) / wc);
	}
	float edge = smoothstep(0.04, 0.08, silhouette);
	if (nearer < 0.04)
		edge = max(edge, smoothstep(0.004, 0.012, crease));
	return edge;
}
void main() {
	vec3 col = texture2D(uColor, vUv).rgb;
	// four sub-pixel taps smooth the stair steps of a per-pixel depth test
	vec2 q = uPixel * 0.35;
	float edge = 0.25 * (edgeAt(vUv + vec2(-q.x, -q.y)) + edgeAt(vUv + vec2(q.x, -q.y)) +
						 edgeAt(vUv + vec2(-q.x, q.y)) + edgeAt(vUv + vec2(q.x, q.y)));
	gl_FragColor = vec4(mix(col, col * vec3(0.1, 0.08, 0.12), edge * 0.92), 1.0);
}
)";

GLuint gProgram = 0;
GLint gLocPixel = -1, gLocWidth = -1, gLocNear = -1, gLocFar = -1;
GLuint gFbo = 0, gColorTex = 0, gDepthTex = 0;
int gWidth = 0, gHeight = 0;
bool gFailed = false;
bool gActive = false;

GLuint compile(GLenum type, const char* src) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	GLint ok = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (ok != GL_TRUE) {
		char log[1024];
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		LOG_ERRORF("graphics", "Ink shader compile failed: %s", log);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

bool buildProgram() {
	GLuint vs = compile(GL_VERTEX_SHADER, VERTEX_SRC);
	GLuint fs = compile(GL_FRAGMENT_SHADER, FRAGMENT_SRC);
	if (vs == 0 || fs == 0)
		return false;
	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glDeleteShader(vs);
	glDeleteShader(fs);
	GLint ok = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (ok != GL_TRUE) {
		char log[1024];
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		LOG_ERRORF("graphics", "Ink shader link failed: %s", log);
		glDeleteProgram(program);
		return false;
	}
	gProgram = program;
	gLocPixel = glGetUniformLocation(program, "uPixel");
	gLocWidth = glGetUniformLocation(program, "uWidth");
	gLocNear = glGetUniformLocation(program, "uNear");
	gLocFar = glGetUniformLocation(program, "uFar");
	glUseProgram(program);
	glUniform1i(glGetUniformLocation(program, "uColor"), 0);
	glUniform1i(glGetUniformLocation(program, "uDepth"), 1);
	glUseProgram(0);
	return true;
}

GLuint makeTexture(GLint internal, GLenum format, GLenum type, int width, int height) {
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, format, type, nullptr);
	return tex;
}

void releaseTarget() {
	glDeleteFramebuffers(1, &gFbo);
	glDeleteTextures(1, &gColorTex);
	glDeleteTextures(1, &gDepthTex);
	gFbo = gColorTex = gDepthTex = 0;
}

// (Re)built on a window size change.
bool ensureTarget(int width, int height) {
	if (gFbo != 0 && width == gWidth && height == gHeight)
		return true;
	releaseTarget();
	gColorTex = makeTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, width, height);
	gDepthTex = makeTexture(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, width, height);
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenFramebuffers(1, &gFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, gFbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gColorTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepthTex, 0);
	bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (!ok) {
		LOG_ERROR("graphics", "Ink framebuffer incomplete, toon outlines off");
		releaseTarget();
		return false;
	}
	gWidth = width;
	gHeight = height;
	return true;
}

// Lazy, so it runs with a current GL context. On failure toon mode draws without outlines.
bool ensureReady(int width, int height) {
	if (gFailed)
		return false;
	if (gProgram == 0 && !buildProgram()) {
		gFailed = true;
		return false;
	}
	if (!ensureTarget(width, height)) {
		gFailed = true;
		return false;
	}
	return true;
}
} // namespace

void Ink::begin(float zNear, float zFar) {
	const int width = Game().render.resX;
	const int height = Game().render.resY;
	gActive = Game().render.Cartoon && width > 0 && height > 0 && ensureReady(width, height);
	if (!gActive)
		return;
	glBindFramebuffer(GL_FRAMEBUFFER, gFbo);
	glUseProgram(gProgram);
	glUniform2f(gLocPixel, 1.f / static_cast<float>(width), 1.f / static_cast<float>(height));
	glUniform1f(gLocWidth, 2.f * static_cast<float>(height) / 720.f); // the same share of the screen at any size
	glUniform1f(gLocNear, zNear);
	glUniform1f(gLocFar, zFar);
	glUseProgram(0);
}

float Ink::figureScale() { return Game().render.Cartoon ? 1.2f : 1.f; }

void Ink::end() {
	if (!gActive)
		return;
	gActive = false;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gDepthTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gColorTex);
	glUseProgram(gProgram);
	glBegin(GL_QUADS); // clip space, the vertex shader ignores the matrices
	glVertex2f(-1.f, -1.f);
	glVertex2f(1.f, -1.f);
	glVertex2f(1.f, 1.f);
	glVertex2f(-1.f, 1.f);
	glEnd();
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glPopAttrib();
}
