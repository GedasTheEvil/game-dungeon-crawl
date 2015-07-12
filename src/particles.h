#ifndef ParticlesH
#define PArticlesH

#define CMaxPart 1000

struct particle
{
     float x, y,z;
     float life;
};

struct rgb
{
     float r,g,b;
};

#include "timer.h"

class ParSys
{
     private :
	   particle pt[CMaxPart];
	   rgb colour;
	   float x, y,z;
	   int life;
	   timer *ani_e, *ani_f;
     public:
	   ParSys();
	   ParSys(int life);
	   ~ParSys();
	   void Fall();
	   void Explode();
	   void Draw();
	   void setCords(float x = 0, float y = 0, float z = 0);
	   void Reset();
};

#endif
