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
constexpr float BLUR_REACH = 0.07f;	   // the farthest sample, as a share of the way to the centre
constexpr float VIGNETTE_DARK = 0.45f; // edge darkness at full strength

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
varying vec2 vUv;
void main() {
	vec2 d = vUv - uCentre;
	float reach = uReach * smoothstep(0.1, 0.6, length(d * vec2(uAspect, 1.0)));
	vec3 sum = vec3(0.0);
	for (int i = 0; i < 12; i++)
		sum += texture2D(uScene, vUv - d * reach * float(i) / 11.0).rgb;
	gl_FragColor = vec4(sum / 12.0, 1.0);
}
)";

const char* const VIGNETTE_SRC = R"(
#version 120
uniform vec2 uCentre;
uniform float uAspect;
uniform float uDark;
varying vec2 vUv;
void main() {
	float r = length((vUv - uCentre) * vec2(uAspect, 1.0)) / uAspect;
	gl_FragColor = vec4(0.0, 0.0, 0.0, uDark * smoothstep(0.35, 0.9, r));
}
)";

struct Program {
	GLuint id = 0;
	GLint centre = -1, aspect = -1, amount = -1;
};

Program gBlur, gVignette;
bool gTried = false;
RenderTarget gTarget;
bool gActive = false;
int gWidth = 0, gHeight = 0;
float gLevel = 0.f; // linear 0..1, eased on the way out (strength())
int gLastMs = -1;

Program build(const char* name, const char* fragmentSrc, const char* amountName) {
	Program p;
	p.id = linkProgram(name, VERTEX_SRC, fragmentSrc);
	if (p.id == 0)
		return p;
	p.centre = glGetUniformLocation(p.id, "uCentre");
	p.aspect = glGetUniformLocation(p.id, "uAspect");
	p.amount = glGetUniformLocation(p.id, amountName);
	return p;
}

// Lazy, so it runs with a current GL context.
void ensurePrograms() {
	if (gTried)
		return;
	gTried = true;
	gBlur = build("Motion blur", BLUR_SRC, "uReach");
	gVignette = build("Vignette", VIGNETTE_SRC, "uDark");
	if (gBlur.id != 0) {
		glUseProgram(gBlur.id);
		glUniform1i(glGetUniformLocation(gBlur.id, "uScene"), 0);
		glUseProgram(0);
	}
}

void fullScreenQuad(const Program& p, float centreX, float centreY, float amount) {
	glUseProgram(p.id);
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
} // namespace

void MotionFx::update(bool on, int nowMs) {
	const int elapsed = gLastMs < 0 ? 0 : std::clamp(nowMs - gLastMs, 0, 100); // a pause or a load is no jump
	gLastMs = nowMs;
	const float step = static_cast<float>(elapsed) / static_cast<float>(EASE_MS);
	gLevel = std::clamp(gLevel + (on ? step : -step), 0.f, 1.f);
}

float MotionFx::strength() { return gLevel * gLevel * (3.f - 2.f * gLevel); }

float MotionFx::fov(float baseDegrees) { return baseDegrees + FOV_KICK * strength(); }

void MotionFx::begin(int width, int height, bool blur) {
	gWidth = width;
	gHeight = height;
	gActive = false;
	if (strength() <= 0.f || !blur)
		return;
	ensurePrograms();
	gActive = gBlur.id != 0 && gTarget.Begin(width, height);
}

void MotionFx::end(float centreX, float centreY) {
	if (strength() <= 0.f)
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
		fullScreenQuad(gBlur, centreX, centreY, BLUR_REACH * strength());
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	ensurePrograms();
	if (gVignette.id != 0) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		fullScreenQuad(gVignette, centreX, centreY, VIGNETTE_DARK * strength());
	}
	glPopAttrib();
}
