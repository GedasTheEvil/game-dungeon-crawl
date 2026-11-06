#ifndef PARTICLES_DRAW_H
#define PARTICLES_DRAW_H

#include "../sim/particles.h"

namespace Particles {
inline bool shown = true; // Options > Display > Blood: off draws no splashes (they still run)
void Draw(const ParticleSystem& system);
} // namespace Particles

#endif
