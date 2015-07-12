#ifndef ItemH
#define ItemH

#include "shader.h"
#include "textures.h"

class item
{
     private:
	   bool in_inventory;
	   CartoonANI *mdl;
	   float x;
          float y;
          Textura tex;
          bool loaded;
	   
     public:
	   int damage ,range ,type, heal, armor, hp;
	   float rotA;
	   float scale;
	   void Draw();
	   bool getPickedUp();
	   item();
	   ~item();
	   bool LoadMDL(const char filename[],Textura &texture, bool compile = 1);
};

#endif
