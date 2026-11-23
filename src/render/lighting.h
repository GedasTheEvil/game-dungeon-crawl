#ifndef LIGHTING_H
#define LIGHTING_H

#include <cstdint>
#include "batch.h"
#include "default_shaders.h"

// Per-pixel point lights over a dark ambient for the gameplay scene (the world shader, GLSL 330 core: texture x vertex
// colour x (ambient + lights); plan gl-core-world). Toon mode (F1) snaps the light to cel
// bands; Ink draws the outlines.
//
// Per frame: begin() -> add() lights -> commit() -> draw the scene -> end(). add() takes positions in the
// current modelview's local space, so callers can add lights from inside their own transforms.
// begin() reads the projection (gProj): an orthographic one (the inventory's models) is lit with the eye along -z.
// A draw lit by it: Batch with `program = Lighting::use`, or a model's Show.
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
// Between begin and end.
[[nodiscard]] bool active();
// The world's batch (plan gl-core-world): its program is use(), its current colour, normal and texture coordinate are
// the world's (as GL's current values were): a model's Show draws in its current colour too.
Batch& batch();
// Batch::Program for a lit draw (plan gl-core-world): binds the world shader with the frame's lights, the toggles
// below, style.tint and the matrices; outside begin..end the default program of style (texture times colour).
bool use(const DefaultShader::Style& style, const Mat4& projection, const Mat4& modelView);

// Draws without lighting (texture x colour) while set: flames, portals, overlays.
void setEmissive(bool on);
// While line.backEdges is set, the lit draws show only their back faces' edges, line.width pixels wide (a model's
// outline, WideLines::LineStyle); a default LineStyle ends it.
void setOutline(const WideLines::LineStyle& line);
// The haze an emissive draw gets (plan gl-core-foundation): a ported draw with the default programs (Batch) takes it
// from here. None while the shader is off.
DefaultShader::Fog emissiveFog();

// Metal shine (plan metallic-shading) while set: the bound texture's alpha is a metal mask (255 matte, lower more
// metal), not see-through; the draw's alpha is the colour's alone. The models set it (AnimatedModel::Show).
void setMetalMask(bool on);

// Model frame blending (plan model-frame-interpolation): the vertex attributes a model draw puts its next key frame's
// positions and normals in.
// Beside DefaultShader's position 0, normal 1, colour 6 and texture coordinate 7.
constexpr unsigned NEXT_POSITION_ATTRIB = 2;
constexpr unsigned NEXT_NORMAL_ATTRIB = 3;
// While the scene is drawn with the shader: the next draws blend their vertices toward the next key frame's by t (0:
// none). False: no shader, a model draws one key frame.
bool setBlend(float t);

// Brightness multiplier around 1 for a fire with this seed at the current game time; 1 with the flicker off.
float flicker(uint32_t seed, float amount);
// Options > Graphics > Light flicker. On at start.
void setFlicker(bool on);

} // namespace Lighting

#endif
