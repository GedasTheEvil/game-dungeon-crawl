#ifndef ANI_c
#define ANI_c

#include "timer.h"

struct VF
{
     float *v;
};

class ANI
{
     protected:
	   timer *frameChange;
          float frame;
          int speed;
          float scale;
          int frameC;
          int texture;
          bool compiled;
          VF *Ver;
          float *Normals;
          float *TexCords;
          int VCount;
          int *List; // compiled list storage
          void Scale(float sc);
          void Translate(float x, float y, float z);
     public:
	   bool bounds;
	   bool loop;
	   ANI();
	   ~ANI();
	   int Load(const char FileName[]);
	   void Show();
	   void Advance_Animation();
	   void setSpeed(int nSpeed);
	   float getScale();
          void BindTexture(int t);
          void Compile();
          void Centrify();
	   void Reset();
};

#endif
