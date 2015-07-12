#ifndef WinLooseH
#define WinLooseH

#include "textures.h"
#include "timer.h"

class winL
{
     private:
	   Textura win;
	   Textura loose;
	   Textura credits;
	   void DrawQuad(float sx, float sy);
	   
     public:
	   winL();
	   void DrawWin();
	   void DrawLoose();
	   void DrawCredits();
};

#endif
