#define GL_GLEXT_PROTOTYPES // GL 2.0 shader entry points, exported by libGL on Linux
#include "motion_fx.h"
#include "render_target.h"
#include "shader.h"

#include <GL/gl.h>
#include <GL/glext.h>
#include <algorithm>

namespace {
constexpr int EASE_MS = 200;
constexpr float FOV_KICK = 6.f;
constexpr float BLUR_REACH = 0.07f; // the farthest sample, as a share of the way to the centre
constexpr float VIGNETTE_DARK = 0.45f; // edge darkness at full strength
constexpr float CINEMA_SEPIA = 0.7f; // a cut-scene's tint: how far the colours go to sepia
constexpr float CINEMA_DARK = 0.55f; // its vignette
constexpr float CINEMA_BARS = 0.08f; // its letterbox bars, each a share of the window's height
constexpr float BLIND_REACH = 0.12f; // a blind's blur (plan giant-cobra-blinding-spit), nearer the centre
constexpr float BLIND_DARK = 0.95f; // its vignette, green-black, round a small clear spot

const char* const VERTEX_SRC = R"(
#version 120
varying vec2 vUv;
void main() {
	vUv = gl_Vertex.xy * 0.5 + 0.5;
	gl_Position = gl_Vertex;
}
)";

// Samples along the line to the centre. The reach grows with the distance, so the player in the middle stays sharp.
const char* const BLUR_SRC = R"(
#version 120
uniform sampler2D uScene;
uniform vec2 uCentre;
uniform float uAspect;
uniform float uReach;
uniform float uSepia;
uniform float uBlind;
varying vec2 vUv;
void main() {
	vec2 d = vUv - uCentre;
	float r = length(d * vec2(uAspect, 1.0));
	float reach = uReach * smoothstep(mix(0.1, 0.03, uBlind), mix(0.6, 0.3, uBlind), r);
	vec3 sum = vec3(0.0);
	for (int i = 0; i < 12; i++)
		sum += texture2D(uScene, vUv - d * reach * float(i) / 11.0).rgb;
	vec3 colour = sum / 12.0;
	float grey = dot(colour, vec3(0.299, 0.587, 0.114));
	colour = mix(colour, grey * vec3(1.08, 0.95, 0.76), uSepia);
	gl_FragColor = vec4(mix(colour, colour * vec3(0.55, 1.0, 0.5), 0.7 * uBlind), 1.0);
}
)";

const char* const VIGNETTE_SRC = R"(
#version 120
uniform vec2 uCentre;
uniform float uAspect;
uniform float uDark;
uniform float uBlind;
varying vec2 vUv;
void main() {
	float r = length((vUv - uCentre) * vec2(uAspect, 1.0)) / uAspect;
	float edge = smoothstep(mix(0.35, 0.05, uBlind), mix(0.9, 0.3, uBlind), r);
	gl_FragColor = vec4(vec3(0.0, 0.08, 0.02) * uBlind, uDark * edge);
}
)";

struct Program {
	GLuint id = 0;
	GLint centre = -1, aspect = -1, amount = -1, sepia = -1, blind = -1;
};

Program gBlur, gVignette;
bool gTried = false;
RenderTarget gTarget;
bool gActive = false;
int gWidth = 0, gHeight = 0;
float gLevel = 0.f; // linear 0..1, eased on the way out (strength())
int gLastMs = -1;
float gCinema = 0.f; // a cut-scene's look, 0..1
float gBlind = 0.f; // the player's blindness, 0..1
bool gToonBlurFirst = false; // toon mode: blur the scene before the ink lines (true) or the finished picture (false)
float gCentreX = 0.5f, gCentreY = 0.5f;

Program build(const char* name, const char* fragmentSrc, const char* amountName) {
	Program p;
	p.id = linkProgram(name, VERTEX_SRC, fragmentSrc);
	if (p.id == 0)
		return p;
	p.centre = glGetUniformLocation(p.id, "uCentre");
	p.aspect = glGetUniformLocation(p.id, "uAspect");
	p.amount = glGetUniformLocation(p.id, amountName);
	p.blind = glGetUniformLocation(p.id, "uBlind");
	return p;
}

// Lazy, so it runs with a current GL context.
void ensurePrograms() {
	if (gTried)
		return;
	gTried = true;
	gBlur = build("Motion blur", BLUR_SRC, "uReach");
	gBlur.sepia = gBlur.id != 0 ? glGetUniformLocation(gBlur.id, "uSepia") : -1;
	gVignette = build("Vignette", VIGNETTE_SRC, "uDark");
	if (gBlur.id != 0) {
		glUseProgram(gBlur.id);
		glUniform1i(glGetUniformLocation(gBlur.id, "uScene"), 0);
		glUseProgram(0);
	}
}

void fullScreenQuad(const Program& p, float centreX, float centreY, float amount) {
	glUseProgram(p.id);
	if (p.sepia >= 0)
		glUniform1f(p.sepia, CINEMA_SEPIA * gCinema);
	if (p.blind >= 0)
		glUniform1f(p.blind, gBlind);
	glUniform2f(p.centre, centreX, centreY);
	glUniform1f(p.aspect, static_cast<float>(gWidth) / static_cast<float>(std::max(gHeight, 1)));
	glUniform1f(p.amount, amount);
	glBegin(GL_QUADS); // clip space, the vertex shader ignores the matrices
	glVertex2f(-1.f, -1.f);
	glVertex2f(1.f, -1.f);
	glVertex2f(1.f, 1.f);
	glVertex2f(-1.f, 1.f);
	glEnd();
	glUseProgram(0);
}

// A cut-scene's letterbox bars, black from the top and bottom edges.
void bars(float share) {
	if (share <= 0.f)
		return;
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glColor3f(0.f, 0.f, 0.f);
	const float h = 2.f * share;
	glBegin(GL_QUADS);
	glVertex2f(-1.f, -1.f);
	glVertex2f(1.f, -1.f);
	glVertex2f(1.f, -1.f + h);
	glVertex2f(-1.f, -1.f + h);
	glVertex2f(-1.f, 1.f - h);
	glVertex2f(1.f, 1.f - h);
	glVertex2f(1.f, 1.f);
	glVertex2f(-1.f, 1.f);
	glEnd();
	glColor3f(1.f, 1.f, 1.f);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}
// Any of the looks is on this frame: the sprint's, a cut-scene's, a blind.
bool anyLook() { return MotionFx::strength() > 0.f || gCinema > 0.f || gBlind > 0.f; }

// The blur's reach this frame: the sprint's or the blind's, the larger.
float blurReach() { return std::max(BLUR_REACH * MotionFx::strength(), BLIND_REACH * gBlind); }
} // namespace

void MotionFx::update(bool on, int nowMs) {
	const int elapsed = gLastMs < 0 ? 0 : std::clamp(nowMs - gLastMs, 0, 100); // a pause or a load is no jump
	gLastMs = nowMs;
	const float step = static_cast<float>(elapsed) / static_cast<float>(EASE_MS);
	gLevel = std::clamp(gLevel + (on ? step : -step), 0.f, 1.f);
}

float MotionFx::strength() { return gLevel * gLevel * (3.f - 2.f * gLevel); }

void MotionFx::cinema(float amount) { gCinema = std::clamp(amount, 0.f, 1.f); }

void MotionFx::blind(float amount) { gBlind = std::clamp(amount, 0.f, 1.f); }

float MotionFx::fov(float baseDegrees) { return baseDegrees + FOV_KICK * strength(); }

void MotionFx::setToonBlurFirst(bool first) { gToonBlurFirst = first; }

bool MotionFx::toonBlurFirst() { return gToonBlurFirst; }

bool MotionFx::blurInInk() {
	if (!gToonBlurFirst || !anyLook())
		return false;
	ensurePrograms();
	return gBlur.id != 0;
}

void MotionFx::blurPass(unsigned sceneTexture) {
	glDisable(GL_BLEND);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sceneTexture);
	fullScreenQuad(gBlur, gCentreX, gCentreY, blurReach());
	glBindTexture(GL_TEXTURE_2D, 0);
}

void MotionFx::setCentre(float centreX, float centreY) {
	gCentreX = centreX;
	gCentreY = centreY;
}

void MotionFx::begin(int width, int height, bool toon) {
	gWidth = width;
	gHeight = height;
	gActive = false;
	if (!anyLook() || (toon && gToonBlurFirst))
		return;
	ensurePrograms();
	gActive = gBlur.id != 0 && gTarget.Begin(width, height);
}

void MotionFx::end() {
	const float centreX = gCentreX, centreY = gCentreY;
	if (!anyLook())
		return;
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	if (gActive) {
		gActive = false;
		gTarget.End();
		glClear(GL_DEPTH_BUFFER_BIT); // the scene's depth went to the target: the HUD must not test against stale depth
		glDisable(GL_BLEND);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(gTarget.TextureID()));
		fullScreenQuad(gBlur, centreX, centreY, blurReach());
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	ensurePrograms();
	if (gVignette.id != 0) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		fullScreenQuad(
			gVignette,
			centreX,
			centreY,
			std::max({VIGNETTE_DARK * strength(), CINEMA_DARK * gCinema, BLIND_DARK * gBlind})
		);
	}
	bars(CINEMA_BARS * gCinema);
	glPopAttrib();
}
