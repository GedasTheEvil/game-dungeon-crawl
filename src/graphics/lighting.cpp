#define GL_GLEXT_PROTOTYPES // GL 2.0 shader entry points, exported by libGL on Linux
#include "lighting.h"
#include "shader.h"

#include <GL/gl.h>
#include <GL/glext.h>
#include <algorithm>
#include <cmath>
#include "../core/logger.h"
#include "../core/timer.h"
#include "ink.h"

namespace {
constexpr int MAX_LIGHTS = 16;

const char* const VERTEX_SRC = R"(
#version 120
uniform float uBlend; // toward the next key frame of a model (plan model-frame-interpolation); 0: gl_Vertex as is
attribute vec3 aNextPos;
attribute vec3 aNextNormal;
varying vec3 vPos;
varying vec3 vNormal;
void main() {
	vec4 pos = gl_Vertex;
	vec3 normal = gl_Normal;
	if (uBlend > 0.0) {
		pos = vec4(mix(gl_Vertex.xyz, aNextPos, uBlend), 1.0);
		normal = normalize(mix(gl_Normal, aNextNormal, uBlend));
	}
	vPos = vec3(gl_ModelViewMatrix * pos);
	vNormal = gl_NormalMatrix * normal;
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = uBlend > 0.0 ? gl_ModelViewProjectionMatrix * pos : ftransform();
}
)";

const char* const FRAGMENT_SRC = R"(
#version 120
const int MAX_LIGHTS = 16;
uniform sampler2D uTex;
uniform vec3 uAmbient;
uniform vec4 uFog; // colour, density per unit past uFogStart
uniform float uFogStart;
uniform float uEmissive;
uniform float uToon; // 1 = cel bands (see toonLight)
// 1 = the texture's alpha is the metal mask (plan metallic-shading): 1 matte, 0 all metal; the draw's alpha is
// gl_Color's alone.
uniform float uMetalMask;
uniform float uOrtho; // 1 = an orthographic view (the inventory): the eye looks down -z at every fragment
uniform int uLightCount;
uniform vec4 uLightPos[MAX_LIGHTS]; // eye space xyz, radius in w
uniform vec3 uLightColor[MAX_LIGHTS];
varying vec3 vPos;
varying vec3 vNormal;
const float METAL_DIFFUSE = 0.7; // what is left of the diffuse light on full metal: it mostly reflects
const float METAL_SHININESS = 10.0; // Blinn-Phong exponent: broad, so small items catch it
const float METAL_SHINE = 2.2; // the highlight's strength, tinted by the metal's colour
const float METAL_TOON_DIFFUSE = 0.75; // toon mode: the bands stay readable, the band does the shine
const float METAL_TOON_BAND = 0.35; // the hard band's light, before METAL_SHINE
// Snaps the light to three flat bands with thin soft steps; the dark band leans cool so shadows read as painted.
vec3 toonLight(vec3 light) {
	float i = max(max(light.r, light.g), light.b);
	vec3 hue = light / max(i, 0.001);
	float w = 0.03;
	float band = 0.12 + 0.33 * smoothstep(0.15 - w, 0.15 + w, i) + 0.55 * smoothstep(0.5 - w, 0.5 + w, i);
	float shade = 1.0 - smoothstep(0.12, 0.45, band);
	return mix(hue, vec3(0.55, 0.6, 1.0), shade * 0.6) * band;
}
void main() {
	vec4 tex = texture2D(uTex, gl_TexCoord[0].st);
	vec4 base = tex * gl_Color;
	float metal = 0.0;
	if (uMetalMask > 0.5) {
		metal = 1.0 - tex.a;
		base.a = gl_Color.a;
	}
	float haze = 1.0 - exp(-uFog.w * max(length(vPos) - uFogStart, 0.0));
	if (uEmissive > 0.5) { // the fog by alpha: a premultiplied sprite's clear corners stay clear (sandstorm-halls)
		gl_FragColor = vec4(mix(base.rgb, uFog.rgb * base.a, haze), base.a);
		return;
	}
	vec3 n = normalize(vNormal);
	vec3 view = uOrtho > 0.5 ? vec3(0.0, 0.0, -1.0) : vPos; // from the eye to the fragment
	if (dot(n, view) > 0.0) // two-sided: the winding in the old models and tiles is not consistent
		n = -n;
	vec3 eye = normalize(-view);
	vec3 light = uAmbient;
	vec3 shine = vec3(0.0);
	for (int i = 0; i < MAX_LIGHTS; i++) {
		if (i >= uLightCount)
			break;
		vec3 d = uLightPos[i].xyz - vPos;
		float dist = length(d);
		vec3 l = d / max(dist, 0.001);
		float k = clamp(1.0 - dist / uLightPos[i].w, 0.0, 1.0);
		float lambert = max(dot(n, l), 0.0) * 0.7 + 0.3; // wrapped, so grazing walls still glow
		light += uLightColor[i] * (k * k * lambert);
		if (metal > 0.0)
			shine += uLightColor[i] * (k * k * pow(max(dot(n, normalize(l + eye)), 0.0), METAL_SHININESS));
	}
	if (uToon > 0.5) {
		float grey = dot(base.rgb, vec3(0.299, 0.587, 0.114));
		base.rgb = clamp(mix(vec3(grey), base.rgb, 1.25), 0.0, 1.0); // livelier paint
		light = toonLight(light) * (1.0 - (1.0 - METAL_TOON_DIFFUSE) * metal); // the metal's bands as the paint's
		float s = max(max(shine.r, shine.g), shine.b); // one hard highlight band
		shine = shine / max(s, 0.001) * smoothstep(0.1, 0.14, s) * METAL_TOON_BAND;
	} else
		light *= 1.0 - (1.0 - METAL_DIFFUSE) * metal;
	vec3 lit = base.rgb * (light + shine * (METAL_SHINE * metal));
	gl_FragColor = vec4(mix(lit, uFog.rgb, haze), base.a);
}
)";

struct Light {
	float x, y, z, radius;
	float r, g, b;
};

GLuint gProgram = 0;
bool gFailed = false;
bool gActive = false;
GLint gLocFog = -1, gLocFogStart = -1;
Lighting::Atmosphere gAir;
GLint gLocAmbient = -1, gLocEmissive = -1, gLocToon = -1, gLocCount = -1, gLocPos = -1, gLocColor = -1;
GLint gLocBlend = -1, gLocMetalMask = -1, gLocOrtho = -1;
Light gLights[64];
int gLightCount = 0;
bool gFlicker = true;

// Lazy, so it runs with a current GL context. On failure the scene stays unlit (fixed pipeline).
bool ensureProgram() {
	if (gProgram != 0)
		return true;
	if (gFailed)
		return false;
	gFailed = true;

	GLuint program = linkProgram(
		"Lighting",
		VERTEX_SRC,
		FRAGMENT_SRC,
		{{Lighting::NEXT_POSITION_ATTRIB, "aNextPos"}, {Lighting::NEXT_NORMAL_ATTRIB, "aNextNormal"}}
	);
	if (program == 0)
		return false;

	gProgram = program;
	gFailed = false;
	gLocAmbient = glGetUniformLocation(program, "uAmbient");
	gLocFog = glGetUniformLocation(program, "uFog");
	gLocFogStart = glGetUniformLocation(program, "uFogStart");
	gLocEmissive = glGetUniformLocation(program, "uEmissive");
	gLocToon = glGetUniformLocation(program, "uToon");
	gLocCount = glGetUniformLocation(program, "uLightCount");
	gLocPos = glGetUniformLocation(program, "uLightPos");
	gLocColor = glGetUniformLocation(program, "uLightColor");
	gLocBlend = glGetUniformLocation(program, "uBlend");
	gLocMetalMask = glGetUniformLocation(program, "uMetalMask");
	gLocOrtho = glGetUniformLocation(program, "uOrtho");
	glUseProgram(program);
	glUniform1i(glGetUniformLocation(program, "uTex"), 0);
	glUseProgram(0);
	LOG_INFO("graphics", "Lighting shader ready");
	return true;
}
} // namespace

void Lighting::begin() {
	gLightCount = 0;
	gActive = ensureProgram();
	if (!gActive)
		return;
	glUseProgram(gProgram);
	glUniform3fv(gLocAmbient, 1, gAir.ambient);
	glUniform4f(gLocFog, gAir.fog[0], gAir.fog[1], gAir.fog[2], gAir.fogDensity);
	glUniform1f(gLocFogStart, gAir.fogStart);
	glUniform1f(gLocEmissive, 0.f);
	glUniform1f(gLocToon, Ink::toon() ? 1.f : 0.f);
	glUniform1f(gLocBlend, 0.f);
	glUniform1f(gLocMetalMask, 0.f);
	float projection[16];
	glGetFloatv(GL_PROJECTION_MATRIX, projection);
	glUniform1f(gLocOrtho, projection[11] == 0.f ? 1.f : 0.f); // a perspective divides by -z: -1 there
}

void Lighting::add(float x, float y, float z, const LightDef& def, uint32_t seed) {
	if (!gActive || gLightCount >= static_cast<int>(sizeof(gLights) / sizeof(gLights[0])))
		return;
	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	float k = flicker(seed, def.flicker);
	gLights[gLightCount++] = {
		m[0] * x + m[4] * y + m[8] * z + m[12],
		m[1] * x + m[5] * y + m[9] * z + m[13],
		m[2] * x + m[6] * y + m[10] * z + m[14],
		def.radius * (0.95f + 0.05f * k),
		def.r * k,
		def.g * k,
		def.b * k
	};
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
		std::partial_sort(gLights, gLights + MAX_LIGHTS, gLights + gLightCount, [&](const Light& a, const Light& b) {
			return dist2(a) < dist2(b);
		});
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

void Lighting::setAtmosphere(const Atmosphere& air) { gAir = air; }

void Lighting::end() {
	if (gActive)
		glUseProgram(0);
	gActive = false;
}

void Lighting::setEmissive(bool on) {
	if (gActive)
		glUniform1f(gLocEmissive, on ? 1.f : 0.f);
}

void Lighting::setMetalMask(bool on) {
	if (gActive)
		glUniform1f(gLocMetalMask, on ? 1.f : 0.f);
}

bool Lighting::setBlend(float t) {
	if (!gActive || gLocBlend < 0)
		return false;
	glUniform1f(gLocBlend, t);
	return true;
}

void Lighting::setFlicker(bool on) { gFlicker = on; }

float Lighting::flicker(uint32_t seed, float amount) {
	if (amount <= 0.f || !gFlicker)
		return 1.f;
	float t = static_cast<float>(GameClock::now()) / 1000.f;
	float s = static_cast<float>(seed % 997U);
	float wave = 0.55f * std::sin(t * 7.3f + s) + 0.3f * std::sin(t * 13.9f + s * 1.7f) +
				 0.15f * std::sin(t * 29.1f + s * 2.3f);
	return 1.f + 0.18f * amount * wave;
}
