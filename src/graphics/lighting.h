#ifndef LIGHTING_H
#define LIGHTING_H

#include <cstdint>

// Per-pixel point lights over a dark ambient for the gameplay scene (GLSL 1.20 program on top of the
// fixed-function pipeline: texture x vertex colour x (ambient + lights)). Toon mode keeps the old unlit path.
//
// Per frame: begin() -> add() lights -> commit() -> draw the scene -> end(). add() takes positions in the
// current modelview's local space, so callers can add lights from inside their own transforms.
namespace Lighting {

struct LightDef {
	float r, g, b;
	float radius;  // world units, the light is zero beyond it
	float flicker; // 0 = steady, 1 = strong fire flicker
};

constexpr LightDef PLAYER = {0.85f, 0.7f, 0.52f, 95.f, 0.f};
constexpr LightDef TORCH = {1.0f, 0.62f, 0.28f, 115.f, 0.6f};
constexpr LightDef BRAZIER = {1.0f, 0.55f, 0.22f, 140.f, 0.8f};
constexpr LightDef OIL_LAMP = {1.0f, 0.68f, 0.35f, 60.f, 0.5f};

void begin();
void add(float x, float y, float z, const LightDef& def, uint32_t seed);
void commit();
void end();

// Draws without lighting (texture x colour) while set: flames, portals, overlays.
void setEmissive(bool on);

// Brightness multiplier around 1 for a fire with this seed at the current game time.
float flicker(uint32_t seed, float amount);

} // namespace Lighting

#endif
