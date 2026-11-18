#include "lighting.h"
#include "shader.h"

#include "gl_includes.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <bit>
#include <cstdint>
#include <string>
#include "../core/logger.h"
#include "../core/timer.h"
#include "gl_legacy.h"
#include "ink.h"
#include "matrix_stack.h"

namespace {
constexpr int MAX_LIGHTS = 16;

// The world shader (plan gl-core-world), GLSL 330 core: Batch's vertices (DefaultShader's attribute locations) or a
// model's vertex array, the matrices as uniforms.
const char* const VERTEX_SRC = R"(
#version 330 core
layout(location = 0) in vec4 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aNextPos; // Lighting::NEXT_POSITION_ATTRIB
layout(location = 3) in vec3 aNextNormal; // Lighting::NEXT_NORMAL_ATTRIB
layout(location = 6) in vec4 aColour;
layout(location = 7) in vec2 aUv;
uniform mat4 uMvp;
uniform mat4 uModelView;
uniform mat3 uNormalMatrix;
uniform vec4 uTint;
uniform float uBlend; // toward the next key frame of a model (plan model-frame-interpolation); 0: aPos as is
out vec3 vPos;
out vec3 vNormal;
out vec4 vColour;
out vec2 vUv;
void main() {
	vec4 pos = aPos;
	vec3 normal = aNormal;
	if (uBlend > 0.0) {
		pos = vec4(mix(aPos.xyz, aNextPos, uBlend), 1.0);
		normal = normalize(mix(aNormal, aNextNormal, uBlend));
	}
	vPos = vec3(uModelView * pos);
	vNormal = uNormalMatrix * normal;
	vColour = clamp(aColour * uTint, 0.0, 1.0); // as the fixed pipeline clamped glColor
	vUv = aUv;
	gl_Position = uMvp * pos;
}
)";

// After the version line.
const char* const FRAGMENT_SRC = R"(
const int MAX_LIGHTS = 16;
uniform sampler2D uTex;
uniform vec3 uAmbient;
uniform vec4 uFog; // colour, density per unit past uFogStart
uniform float uFogStart;
uniform float uEmissive;
uniform float uToon; // 1 = cel bands (see toonLight)
// 1 = the texture's alpha is the metal mask (plan metallic-shading): 1 matte, 0 all metal; the draw's alpha is
// vColour's alone.
uniform float uMetalMask;
uniform float uAlphaRef; // a fragment at or below it is dropped (glAlphaFunc(GL_GREATER)); -1: none
uniform float uOrtho; // 1 = an orthographic view (the inventory): the eye looks down -z at every fragment
uniform int uLightCount;
uniform vec4 uLightPos[MAX_LIGHTS]; // eye space xyz, radius in w
uniform vec3 uLightColor[MAX_LIGHTS];
in vec3 vPos;
in vec3 vNormal;
in vec4 vColour;
in vec2 vUv;
out vec4 fragColour;
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
	vec4 tex = texture(uTex, vUv);
	vec4 base = tex * vColour;
	float metal = 0.0;
	if (uMetalMask > 0.5) {
		metal = 1.0 - tex.a;
		base.a = vColour.a;
	}
	float haze = 1.0 - exp(-uFog.w * max(length(vPos) - uFogStart, 0.0));
	if (base.a <= uAlphaRef)
		discard;
	if (uEmissive > 0.5) { // the fog by alpha: a premultiplied sprite's clear corners stay clear (sandstorm-halls)
		fragColour = vec4(mix(base.rgb, uFog.rgb * base.a, haze), base.a);
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
	fragColour = vec4(mix(lit, uFog.rgb, haze), base.a);
}
)";

struct Light {
	float x, y, z, radius;
	float r, g, b;
};

// A build of the lighting program and its uniforms' locations.
struct Program {
	GLuint id = 0;
	GLint ambient = -1, fog = -1, fogStart = -1, emissive = -1, toon = -1, count = -1, pos = -1, colour = -1;
	GLint blend = -1, metalMask = -1, ortho = -1, alphaRef = -1;
	GLint mvp = -1, modelView = -1, normalMatrix = -1, tint = -1; // the core one's
};

// What the next draws are lit with: set by begin, commit and the toggles, uploaded to the program that draws.
struct State {
	float ortho = 0.f;
	float emissive = 0.f;
	float metalMask = 0.f;
	float blend = 0.f;
	int lightCount = 0;
	float pos[MAX_LIGHTS * 4] = {};
	float colour[MAX_LIGHTS * 3] = {};
};

Program gCore; // Lighting::use
bool gFailed = false;
bool gActive = false;
Lighting::Atmosphere gAir;
State gState;
bool gCoreFresh = false; // the core program has the frame's atmosphere and lights (begin and commit clear it)
// What the core program's per-draw uniforms hold, so a draw sets only what changed: most draws in a row share the
// matrices and the toggles (each upload costs the software renderer a copy of its constants).
struct Uploaded {
	bool valid = false;
	Mat4 projection, modelView;
	std::array<float, 4> tint{};
	float emissive = 0.f, metalMask = 0.f, blend = 0.f, alphaRef = 0.f;
};
Uploaded gUploaded;
Light gLights[64];
int gLightCount = 0;
bool gFlicker = true;

Program build(const char* name, const char* vertexSrc, const char* version) {
	Program p;
	const std::string fragment = std::string(version) + FRAGMENT_SRC;
	p.id = linkProgram(name, vertexSrc, fragment.c_str());
	if (p.id == 0)
		return p;
	auto at = [&p](const char* uniform) { return glGetUniformLocation(p.id, uniform); };
	p.ambient = at("uAmbient");
	p.fog = at("uFog");
	p.fogStart = at("uFogStart");
	p.emissive = at("uEmissive");
	p.toon = at("uToon");
	p.count = at("uLightCount");
	p.pos = at("uLightPos");
	p.colour = at("uLightColor");
	p.blend = at("uBlend");
	p.metalMask = at("uMetalMask");
	p.ortho = at("uOrtho");
	p.alphaRef = at("uAlphaRef");
	p.mvp = at("uMvp");
	p.modelView = at("uModelView");
	p.normalMatrix = at("uNormalMatrix");
	p.tint = at("uTint");
	glUseProgram(p.id);
	glUniform1i(at("uTex"), 0);
	glUniform1f(p.alphaRef, -1.f);
	glUseProgram(0);
	return p;
}

// Lazy, so it runs with a current GL context. On failure the scene stays unlit (the default programs).
bool ensureProgram() {
	if (gCore.id != 0)
		return true;
	if (gFailed)
		return false;
	gFailed = true;
	gCore = build("Lighting", VERTEX_SRC, "#version 330 core\n");
	if (gCore.id == 0)
		return false;
	gFailed = false;
	LOG_INFO("graphics", "Lighting shader ready");
	return true;
}

// The frame's atmosphere and lights into p (bound).
void uploadFrame(const Program& p) {
	glUniform3fv(p.ambient, 1, gAir.ambient);
	glUniform4f(p.fog, gAir.fog[0], gAir.fog[1], gAir.fog[2], gAir.fogDensity);
	glUniform1f(p.fogStart, gAir.fogStart);
	glUniform1f(p.toon, Ink::toon() ? 1.f : 0.f);
	glUniform1f(p.ortho, gState.ortho);
	glUniform1i(p.count, gState.lightCount);
	if (gState.lightCount > 0) {
		glUniform4fv(p.pos, gState.lightCount, gState.pos);
		glUniform3fv(p.colour, gState.lightCount, gState.colour);
	}
}

// One uniform float into the bound program, unless it holds it already.
void setFloat(GLint location, float& held, float value) {
	if (gUploaded.valid && held == value)
		return;
	held = value;
	glUniform1f(location, value);
}

// The inverse transpose of m's upper 3 x 3 (gl_NormalMatrix), column-major.
std::array<float, 9> normalMatrix(const Mat4& m) {
	const float a = m.at(0, 0), b = m.at(0, 1), c = m.at(0, 2);
	const float d = m.at(1, 0), e = m.at(1, 1), f = m.at(1, 2);
	const float g = m.at(2, 0), h = m.at(2, 1), k = m.at(2, 2);
	const float c00 = e * k - f * h, c01 = f * g - d * k, c02 = d * h - e * g;
	const float det = a * c00 + b * c01 + c * c02;
	const float s = det != 0.f ? 1.f / det : 0.f;
	// (M^-1)^T = the cofactors over det; column-major, element (row r, column c) at [c * 3 + r].
	return {
		c00 * s,
		(c * h - b * k) * s,
		(b * f - c * e) * s,
		c01 * s,
		(a * k - c * g) * s,
		(c * d - a * f) * s,
		c02 * s,
		(b * g - a * h) * s,
		(a * e - b * d) * s
	};
}
} // namespace

void Lighting::begin() {
	gLightCount = 0;
	gActive = ensureProgram();
	if (!gActive)
		return;
	const GlLegacy::PortedScope ported;
	gState = {};
	gState.ortho = gProj.top().m[11] == 0.f ? 1.f : 0.f; // a perspective divides by -z: -1 there
	gCoreFresh = false;
}

void Lighting::add(float x, float y, float z, const LightDef& def, uint32_t seed) {
	if (!gActive || gLightCount >= static_cast<int>(sizeof(gLights) / sizeof(gLights[0])))
		return;
	const GlLegacy::PortedScope ported;
	const Vec4 eye = gMv.top() * Vec4{x, y, z, 1.f};
	float k = flicker(seed, def.flicker);
	gLights[gLightCount++] = {eye.x, eye.y, eye.z, def.radius * (0.95f + 0.05f * k), def.r * k, def.g * k, def.b * k};
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
	float* p = gState.pos;
	float* c = gState.colour;
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
	gState.lightCount = gLightCount;
	gCoreFresh = false;
}

void Lighting::setAtmosphere(const Atmosphere& air) { gAir = air; }

void Lighting::end() { gActive = false; }

bool Lighting::active() { return gActive; }

Batch& Lighting::batch() {
	static Batch world;
	world.program = use;
	return world;
}

bool Lighting::use(const DefaultShader::Style& style, const Mat4& projection, const Mat4& modelView) {
	if (!gActive)
		return DefaultShader::use(style, projection, modelView); // unlit: texture times colour
	glUseProgram(gCore.id);
	if (!gCoreFresh) {
		uploadFrame(gCore);
		gCoreFresh = true;
	}
	Uploaded& held = gUploaded;
	setFloat(gCore.emissive, held.emissive, gState.emissive);
	setFloat(gCore.metalMask, held.metalMask, gState.metalMask);
	setFloat(gCore.blend, held.blend, gState.blend);
	setFloat(gCore.alphaRef, held.alphaRef, style.kind == DefaultShader::Kind::AlphaTest ? style.alphaRef : -1.f);
	const std::array<float, 4> tint{style.tint[0], style.tint[1], style.tint[2], style.tint[3]};
	if (!held.valid || tint != held.tint) {
		held.tint = tint;
		glUniform4fv(gCore.tint, 1, tint.data());
	}
	// Bit for bit: a -0 is another matrix to the rasteriser's rounding.
	auto same = [](const Mat4& a, const Mat4& b) {
		for (size_t i = 0; i < 16; i++)
			if (std::bit_cast<std::uint32_t>(a.m[i]) != std::bit_cast<std::uint32_t>(b.m[i]))
				return false;
		return true;
	};
	if (!held.valid || !same(projection, held.projection) || !same(modelView, held.modelView)) {
		held.projection = projection;
		held.modelView = modelView;
		const Mat4 mvp = projection * modelView;
		glUniformMatrix4fv(gCore.mvp, 1, GL_FALSE, mvp.data());
		glUniformMatrix4fv(gCore.modelView, 1, GL_FALSE, modelView.data());
		glUniformMatrix3fv(gCore.normalMatrix, 1, GL_FALSE, normalMatrix(modelView).data());
	}
	held.valid = true;
	return true;
}

void Lighting::setEmissive(bool on) { gState.emissive = on ? 1.f : 0.f; }

DefaultShader::Fog Lighting::emissiveFog() {
	if (!gActive)
		return {};
	return {gAir.fog[0], gAir.fog[1], gAir.fog[2], gAir.fogDensity, gAir.fogStart};
}

void Lighting::setMetalMask(bool on) { gState.metalMask = on ? 1.f : 0.f; }

bool Lighting::setBlend(float t) {
	if (!gActive)
		return false;
	gState.blend = t;
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
