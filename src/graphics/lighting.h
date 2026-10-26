#ifndef LIGHTING_H
#define LIGHTING_H

#include <cstdint>

// Per-pixel point lights over a dark ambient for the gameplay scene (GLSL 1.20 program on top of the
// fixed-function pipeline: texture x vertex colour x (ambient + lights)). Toon mode (F1) snaps the light to cel
// bands; Ink draws the outlines.
//
// Per frame: begin() -> add() lights -> commit() -> draw the scene -> end(). add() takes positions in the
// current modelview's local space, so callers can add lights from inside their own transforms.
namespace Lighting {

struct LightDef {
	float r, g, b;
	float radius; // world units, the light is zero beyond it
	float flicker; // 0 = steady, 1 = strong fire flicker
};

constexpr LightDef PLAYER = {0.85f, 0.7f, 0.52f, 95.f, 0.f};
constexpr LightDef TORCH = {1.0f, 0.62f, 0.28f, 115.f, 0.6f};
constexpr LightDef BRAZIER = {1.0f, 0.55f, 0.22f, 140.f, 0.8f};
constexpr LightDef OIL_LAMP = {1.0f, 0.68f, 0.35f, 60.f, 0.5f};
constexpr LightDef LIGHT_SHAFT = {0.9f, 0.78f, 0.55f, 80.f, 0.f}; // daylight through a crack (plan desert-chapter)
constexpr LightDef SUNBEAM =
	{1.0f, 0.86f, 0.6f, 75.f, 0.f}; // a beam of sunlight, a lit sun disc (plan sunlight-mirrors)

// The air of the level's zone (plan zone-looks, ZoneLook): the light where no lamp reaches, the haze far away.
struct Atmosphere {
	float ambient[3] = {0.09f, 0.085f, 0.1f};
	float fog[3] = {0.f, 0.f, 0.f};
	float fogDensity = 0.f; // per world unit past fogStart; 0: none
	float fogStart = 0.f; // eye distance
};

// Per frame, before begin(): the atmosphere the next frames are drawn in.
void setAtmosphere(const Atmosphere& air);

void begin();
void add(float x, float y, float z, const LightDef& def, uint32_t seed);
void commit();
void end();

// Draws without lighting (texture x colour) while set: flames, portals, overlays.
void setEmissive(bool on);

// Model frame blending (plan model-frame-interpolation): the generic attributes a model draw puts its next key
// frame's positions and normals in.
constexpr unsigned NEXT_POSITION_ATTRIB = 6; // free of the built-ins' aliases (NVIDIA: 0, 2, 3, 8-15)
constexpr unsigned NEXT_NORMAL_ATTRIB = 7;
// While the scene is drawn with the shader: the next draws blend their vertices toward the next key frame's by t (0:
// none). False: no shader, a model draws one key frame.
bool setBlend(float t);

// Brightness multiplier around 1 for a fire with this seed at the current game time; 1 with the flicker off.
float flicker(uint32_t seed, float amount);
// Options > Display > Light flicker. On at start.
void setFlicker(bool on);

} // namespace Lighting

#endif
