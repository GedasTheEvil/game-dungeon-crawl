#ifndef TrapsH
#define TrapsH

#include "shader.h"
#include "textures.h"
#include "timer.h"

class trap
{
     private:
	   CartoonANI *mdl;
	   float x;
	   float y;
	   
	   Textura tex;
	   
     public:
	   float scale;
	   timer *Hurt_timer;
	   float *DY;//ne , ne isvestine :D. Kordinates pozemio
	   float *DX;
	   
	   trap();
	   ~trap();
	   void Show();
	   void Hurt();
	   void setCords(float nX, float nY);
	   bool LoadMDL(const char filename[],Textura &texture, bool compile = 1);
	   void debugText();
};

#endif
