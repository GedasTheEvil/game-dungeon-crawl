#ifndef PARTICLES_H
#define PARTICLES_H

#include "../core/timer.h"

struct Rgb {
	float r, g, b;
};

// A blood splash: particles burst from (x, y, z) and drift down while the system has life left.
class ParticleSystem {
  private:
	static constexpr int PARTICLE_COUNT = 1000;
	static constexpr int DEFAULT_LIFE = 100; // of the system and of a particle, in steps
	struct Particle {
		float x = 0.f, y = 0.f, z = 0.f;
		float life = DEFAULT_LIFE;
	};
	Particle pt[PARTICLE_COUNT];
	Rgb colour = {0.7f, 0.1f, 0.1f};
	float x = 0.f, y = 0.f, z = 0.f;
	int life;
	Timer frameTimer{5}, decayTimer{5};

  public:
	explicit ParticleSystem(int life = DEFAULT_LIFE) : life(life) {}
	void Fall();
	void Explode();
	void Draw();
	void setCords(float x = 0, float y = 0, float z = 0);
	void setBloodColor(float r, float g, float b);
	void Reset(); // full life again: a new splash
	void Stop();
};

#endif
