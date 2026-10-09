#ifndef ENTITIES_PARTICLES_H
#define ENTITIES_PARTICLES_H

#include "../core/timer.h"
#include "../world/rgb.h"
#include "../world/rng.h"

// A blood splash: particles burst from (x, y, z) and drift down while the system has life left. Only the motion, no
// GL: Particles::Draw (graphics/particles.h) draws it.
class ParticleSystem {
  public:
	static constexpr int PARTICLE_COUNT = 1000;
	static constexpr int DEFAULT_LIFE = 100; // of the system and of a particle, in steps
	struct Particle {
		float x = 0.f, y = 0.f, z = 0.f;
		float life = DEFAULT_LIFE;
	};

  private:
	Particle pt[PARTICLE_COUNT];
	Rgb colour = {0.7f, 0.1f, 0.1f};
	float x = 0.f, y = 0.f, z = 0.f;
	int life;
	Timer frameTimer{5}, decayTimer{5};
	Rng rng; // its own stream: the splashes never shift the game's rolls (loot, riddles)
	static inline uint64_t instances = 0;

  public:
	explicit ParticleSystem(int life = DEFAULT_LIFE) : life(life), rng(++instances) {}
	// A new splash at a random point within `extent` of the origin (x and y), at full life.
	void Splash(int extent);
	void Fall();
	void Explode();
	void setCords(float x = 0, float y = 0, float z = 0);
	void setBloodColor(float r, float g, float b);
	void Reset(); // full life again: a new splash
	void Stop();
	[[nodiscard]] bool Live() const { return life > 0; }
	[[nodiscard]] const Particle* Points() const { return pt; }
	[[nodiscard]] const Rgb& Colour() const { return colour; }
	[[nodiscard]] float X() const { return x; }
	[[nodiscard]] float Y() const { return y; }
	[[nodiscard]] float Z() const { return z; }
};

#endif
