#ifndef FIRE_H
#define FIRE_H

#include <cstdint>

// Fire particle system: camera-facing sprites that rise from a point, yellow -> orange -> red -> dark grey smoke,
// then fade out. Stateless: every particle is a function of (seed, index, game time), so any number of fires cost
// no memory or update step, and scenario runs render the same frames.
struct FireStyle {
	int count;		   // particles alive at once
	float lifeMs;	   // average particle life
	float spawnRadius; // world units around the origin
	float height;	   // how far a particle rises over its life
	float size;		   // sprite side at birth, world units
	float sway;		   // sideways drift of the smoke
	float smoke;	   // smoke opacity, 0..1
};

namespace Fire {
constexpr FireStyle TORCH = {40, 900.f, 1.4f, 15.f, 3.4f, 2.5f, 0.75f};
constexpr FireStyle BRAZIER = {72, 1100.f, 3.5f, 20.f, 4.4f, 3.5f, 0.9f};
constexpr FireStyle OIL_LAMP = {14, 600.f, 0.4f, 5.f, 1.6f, 0.8f, 0.3f};

// Draws one fire at (x, y, z) in the current modelview's local space. Call after the opaque scene: the sprites
// test depth but do not write it.
void draw(const FireStyle& style, float x, float y, float z, uint32_t seed);
} // namespace Fire

// Dust and grit trickling from a loose ceiling (rock-fall warning). Stateless like the fire.
namespace Dust {
// (x, y, z) = the ceiling point in local space; progress 0..1 over the warning, the trickle thickens towards 1.
void draw(float x, float y, float z, float progress, uint32_t seed);
} // namespace Dust

#endif
