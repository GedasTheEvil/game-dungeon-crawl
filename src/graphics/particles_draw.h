#ifndef PARTICLES_DRAW_H
#define PARTICLES_DRAW_H

#include "../sim/particles.h"

namespace Particles {
inline bool shown = true; // Options > Graphics > Blood: off draws no splashes (they still run)
void Draw(const ParticleSystem& system);
// Bleeding (plan bleeding): a few red drops fall from the body's middle to its feet while it bleeds, placed by the game
// clock (no state). In the body's scaled space: height is its top; seed spreads the bodies apart. Off with the blood.
void DrawBleedDrops(float height, int seed);
} // namespace Particles

#endif
