#ifndef PARTICLES_H
#define PARTICLES_H
#define CMaxPart 1000

struct BloodParticle {
	float x, y, z;
	float life;
};

struct Rgb {
	float r, g, b;
};

#include <memory>
#include "../core/timer.h"

class ParticleSystem {
  private:
	BloodParticle pt[CMaxPart];
	Rgb colour;
	float x = 0.f, y = 0.f, z = 0.f;
	int life;
	std::unique_ptr<Timer> frameTimer, decayTimer;

  public:
	ParticleSystem();
	ParticleSystem(int life);
	~ParticleSystem();
	void Fall();
	void Explode();
	void Draw();
	void setCords(float x = 0, float y = 0, float z = 0);
	void setBloodColor(float r, float g, float b);
	void Reset();
	void Stop();
};

#endif
