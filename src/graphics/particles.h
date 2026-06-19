#ifndef ParticlesH
#define PArticlesH

#define CMaxPart 1000

struct particle {
	float x, y, z;
	float life;
};

struct rgb {
	float r, g, b;
};

#include <memory>
#include "../core/timer.h"

class ParSys {
  private:
	particle pt[CMaxPart];
	rgb colour;
	float x, y, z;
	int life;
	std::unique_ptr<timer> frameTimer, decayTimer;

  public:
	ParSys();
	ParSys(int life);
	~ParSys();
	void Fall();
	void Explode();
	void Draw();
	void setCords(float x = 0, float y = 0, float z = 0);
	void setBloodColor(float r, float g, float b);
	void Reset();
};

#endif
