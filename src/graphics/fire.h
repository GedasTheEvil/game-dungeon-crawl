#ifndef FIRE_H
#define FIRE_H

#include <cstdint>

// Fire particle system: camera-facing sprites that rise from a point, yellow -> orange -> red -> dark grey smoke,
// then fade out. Stateless: every particle is a function of (seed, index, game time), so any number of fires cost
// no memory or update step, and scenario runs render the same frames.
// One colour stop of a fire's ramp over a particle's life (0..1). add = 1 blends additively (glowing flame), 0 blends
// over (smoke that darkens).
struct FireKey {
	float p, r, g, b, a, add;
};
struct FireRamp {
	const FireKey* keys;
	int count;
};
namespace Fire {
extern const FireRamp FLAME_RAMP; // white-yellow, yellow, orange, red, embers, grey smoke
extern const FireRamp VENOM_RAMP; // the same shape in greens, close to VENOM_GREEN
} // namespace Fire

struct FireStyle {
	int count; // particles alive at once
	float lifeMs; // average particle life
	float spawnRadius; // world units around the origin
	float height; // how far a particle rises over its life
	float size; // sprite side at birth, world units
	float sway; // sideways drift of the smoke
	float smoke; // smoke opacity, 0..1
	const FireRamp* ramp = &Fire::FLAME_RAMP; // colours over a particle's life
};

namespace Fire {
constexpr FireStyle TORCH = {40, 900.f, 1.4f, 15.f, 3.4f, 2.5f, 0.75f};
constexpr FireStyle BRAZIER = {72, 1100.f, 3.5f, 20.f, 4.4f, 3.5f, 0.9f};
constexpr FireStyle OIL_LAMP = {14, 600.f, 0.4f, 5.f, 1.6f, 0.8f, 0.3f};
// The cobras' venom glob: a fuzzy green cloud of short-lived flames (no ball), a little smoke; smaller ones trail it.
constexpr FireStyle VENOM = {20, 420.f, 1.4f, 5.f, 2.8f, 1.2f, 0.35f, &VENOM_RAMP};
constexpr FireStyle VENOM_TRAIL = {10, 320.f, 1.f, 3.5f, 2.2f, 1.f, 0.4f, &VENOM_RAMP};

// A fire arrow's tip, a burning body's tongues of flame (plan fire-damage).
constexpr FireStyle ARROW = {16, 380.f, 0.5f, 4.f, 2.f, 1.f, 0.3f};
constexpr FireStyle BODY = {24, 600.f, 2.5f, 9.f, 3.6f, 1.5f, 0.4f};
// The Anubis's fire bolt (plan anubis-fire-bolt): a small fireball, about an oil lamp's flame, short-lived and with
// little smoke, smaller clouds trailing it; a short burst where it hits a wall or the floor.
constexpr FireStyle BOLT = {20, 380.f, 0.9f, 4.f, 2.6f, 1.f, 0.15f};
constexpr FireStyle BOLT_TRAIL = {10, 300.f, 0.8f, 3.f, 2.f, 0.8f, 0.2f};
constexpr FireStyle BOLT_SPLAT = {40, 450.f, 2.5f, 9.f, 3.4f, 2.f, 0.5f};

// Draws one fire at (x, y, z) in the current modelview's local space. Call after the opaque scene: the sprites
// test depth but do not write it. fade 0..1 dims it (1: full).
void draw(const FireStyle& style, float x, float y, float z, uint32_t seed, float fade = 1.f);
// A burning body: BODY flames over its box, bottom centre (x, y, z), halfWidth and height in world units.
void body(float x, float y, float z, float halfWidth, float height, uint32_t seed);
} // namespace Fire

// Dust and grit trickling from a loose ceiling (rock-fall warning). Stateless like the fire.
namespace Dust {
// (x, y, z) = the ceiling point in local space; progress 0..1 over the warning, the trickle thickens towards 1.
void draw(float x, float y, float z, float progress, uint32_t seed);
} // namespace Dust

// A boss's minion comes out: sand sprays up from the floor where it digs out, or grit and dust fall from the ceiling
// where it drops. Stateless like the fire.
namespace Grit {
constexpr int BURST_MS = 1100;
// (x, y, z) = the floor point (or the ceiling point, fromCeiling) in local space; ageMs since the summon, < BURST_MS.
void burst(float x, float y, float z, int ageMs, bool fromCeiling, uint32_t seed);
// A step in sand (plan desert-chapter): a small puff of dust at the feet, (x, y, z) the floor point; ageMs < lifeMs.
void puff(float x, float y, float z, int ageMs, int lifeMs, uint32_t seed);
// A sand fall pouring down from the ceiling point (x, y, z) to the floor a tile below, ageMs since it began, for
// lifeMs: a thick stream of grains, thinning at the end, and dust where it lands.
void pour(float x, float y, float z, int ageMs, int lifeMs, uint32_t seed);
} // namespace Grit

// Level-up blessing: a shaft of sunlight falls on the player, gold motes rise through it. Stateless like the fire.
namespace SunBeam {
constexpr int DURATION_MS = 1600;
// 0..1..0 brightness over the effect, progress 0..1.
float strength(float progress);
// (x, y, z) = the player's feet in local space. Call after the opaque scene, like the fire.
void draw(float x, float y, float z, float progress);
// A shaft of daylight through a crack in the ceiling (plan desert-chapter): steady and faint, from the ceiling a tile
// above (x, y, z) down to that floor point, dust motes drifting in it.
void shaft(float x, float y, float z, uint32_t seed);
} // namespace SunBeam

#endif
