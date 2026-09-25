#define GL_GLEXT_PROTOTYPES // GL 2.0 shader entry points, exported by libGL on Linux
#include "lighting.h"

#include <GL/gl.h>
#include <GL/glext.h>
#include <algorithm>
#include <cmath>
#include "../core/logger.h"
#include "../core/service_locator.h"
#include "../core/timer.h"
#include "../state/game_state.h"

namespace {
constexpr int MAX_LIGHTS = 16;
constexpr float AMBIENT[3] = {0.09f, 0.085f, 0.1f}; // cool, dark; torches and the player do the rest

const char* const VERTEX_SRC = R"(
#version 120
varying vec3 vPos;
varying vec3 vNormal;
void main() {
	vPos = vec3(gl_ModelViewMatrix * gl_Vertex);
	vNormal = gl_NormalMatrix * gl_Normal;
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}
)";

const char* const FRAGMENT_SRC = R"(
#version 120
const int MAX_LIGHTS = 16;
uniform sampler2D uTex;
uniform vec3 uAmbient;
uniform float uEmissive;
uniform int uLightCount;
uniform vec4 uLightPos[MAX_LIGHTS]; // eye space xyz, radius in w
uniform vec3 uLightColor[MAX_LIGHTS];
varying vec3 vPos;
varying vec3 vNormal;
void main() {
	vec4 base = texture2D(uTex, gl_TexCoord[0].st) * gl_Color;
	if (uEmissive > 0.5) {
		gl_FragColor = base;
		return;
	}
	vec3 n = normalize(vNormal);
	if (dot(n, vPos) > 0.0) // two-sided: the winding in the old models and tiles is not consistent
		n = -n;
	vec3 light = uAmbient;
	for (int i = 0; i < MAX_LIGHTS; i++) {
		if (i >= uLightCount)
			break;
		vec3 d = uLightPos[i].xyz - vPos;
		float dist = length(d);
		float k = clamp(1.0 - dist / uLightPos[i].w, 0.0, 1.0);
		float lambert = max(dot(n, d / max(dist, 0.001)), 0.0) * 0.7 + 0.3; // wrapped, so grazing walls still glow
		light += uLightColor[i] * (k * k * lambert);
	}
	gl_FragColor = vec4(base.rgb * light, base.a);
}
)";

struct Light {
	float x, y, z, radius;
	float r, g, b;
};

GLuint gProgram = 0;
bool gFailed = false;
bool gActive = false;
GLint gLocAmbient = -1, gLocEmissive = -1, gLocCount = -1, gLocPos = -1, gLocColor = -1;
Light gLights[64];
int gLightCount = 0;

GLuint compile(GLenum type, const char* src) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	GLint ok = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (ok != GL_TRUE) {
		char log[1024];
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		LOG_ERRORF("graphics", "Lighting shader compile failed: %s", log);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

// Lazy, so it runs with a current GL context. On failure the scene stays unlit (fixed pipeline).
bool ensureProgram() {
	if (gProgram != 0)
		return true;
	if (gFailed)
		return false;
	gFailed = true;

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
		LOG_ERRORF("graphics", "Lighting shader link failed: %s", log);
		glDeleteProgram(program);
		return false;
	}

	gProgram = program;
	gFailed = false;
	gLocAmbient = glGetUniformLocation(program, "uAmbient");
	gLocEmissive = glGetUniformLocation(program, "uEmissive");
	gLocCount = glGetUniformLocation(program, "uLightCount");
	gLocPos = glGetUniformLocation(program, "uLightPos");
	gLocColor = glGetUniformLocation(program, "uLightColor");
	glUseProgram(program);
	glUniform1i(glGetUniformLocation(program, "uTex"), 0);
	glUseProgram(0);
	LOG_INFO("graphics", "Lighting shader ready");
	return true;
}
} // namespace

void Lighting::begin() {
	gLightCount = 0;
	gActive = !GAME_STATE.render.Cartoon && ensureProgram();
	if (!gActive)
		return;
	glUseProgram(gProgram);
	glUniform3fv(gLocAmbient, 1, AMBIENT);
	glUniform1f(gLocEmissive, 0.f);
}

void Lighting::add(float x, float y, float z, const LightDef& def, uint32_t seed) {
	if (!gActive || gLightCount >= static_cast<int>(sizeof(gLights) / sizeof(gLights[0])))
		return;
	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	float k = flicker(seed, def.flicker);
	gLights[gLightCount++] = {m[0] * x + m[4] * y + m[8] * z + m[12],
							  m[1] * x + m[5] * y + m[9] * z + m[13],
							  m[2] * x + m[6] * y + m[10] * z + m[14],
							  def.radius * (0.95f + 0.05f * k),
							  def.r * k,
							  def.g * k,
							  def.b * k};
}

// Over MAX_LIGHTS, the ones nearest to the first light added (the player's) win.
void Lighting::commit() {
	if (!gActive)
		return;
	if (gLightCount > MAX_LIGHTS) {
		Light focus = gLights[0];
		auto dist2 = [&](const Light& l) {
			return (l.x - focus.x) * (l.x - focus.x) + (l.y - focus.y) * (l.y - focus.y) +
				   (l.z - focus.z) * (l.z - focus.z);
		};
		std::partial_sort(gLights, gLights + MAX_LIGHTS, gLights + gLightCount,
						  [&](const Light& a, const Light& b) { return dist2(a) < dist2(b); });
		gLightCount = MAX_LIGHTS;
	}
	float pos[MAX_LIGHTS * 4];
	float col[MAX_LIGHTS * 3];
	float* p = pos;
	float* c = col;
	for (int i = 0; i < gLightCount; i++) {
		const Light& l = gLights[i];
		*p++ = l.x;
		*p++ = l.y;
		*p++ = l.z;
		*p++ = l.radius;
		*c++ = l.r;
		*c++ = l.g;
		*c++ = l.b;
	}
	glUniform1i(gLocCount, gLightCount);
	if (gLightCount > 0) {
		glUniform4fv(gLocPos, gLightCount, pos);
		glUniform3fv(gLocColor, gLightCount, col);
	}
}

void Lighting::end() {
	if (gActive)
		glUseProgram(0);
	gActive = false;
}

void Lighting::setEmissive(bool on) {
	if (gActive)
		glUniform1f(gLocEmissive, on ? 1.f : 0.f);
}

float Lighting::flicker(uint32_t seed, float amount) {
	if (amount <= 0.f)
		return 1.f;
	float t = static_cast<float>(GameClock::now()) / 1000.f;
	float s = static_cast<float>(seed % 997U);
	float wave =
		0.55f * std::sin(t * 7.3f + s) + 0.3f * std::sin(t * 13.9f + s * 1.7f) + 0.15f * std::sin(t * 29.1f + s * 2.3f);
	return 1.f + 0.18f * amount * wave;
}
