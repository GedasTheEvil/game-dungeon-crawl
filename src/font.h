#ifndef Font_H
#define Font_H

#include "textures.h"

class Font
{
     private:
	   Textura t;
// 	   int loaded;
	   int base;
     public:
	   Font();
	   ~Font();
	   void Load(const char filename[], float size = 12, float spacing = 0);
	   void print(int x, int y, const char *string, ...);
};

#endif