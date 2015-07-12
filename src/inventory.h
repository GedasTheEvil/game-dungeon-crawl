#ifndef InventoryH
#define InventoryH

#include "font.h" 
#include "item.h"
#include "fstream"
#include "timer.h"

struct InvItem
{
     int id;
     int count;
     float realScale;
};

struct eq
{
     int type;
     int id;
     int count;
};

class inventory
{
     private:
	   InvItem mw[3]; //melea weapon
	   InvItem rw;  //ranged weapon
	   eq equipped;
	   eq view;
	   InvItem potions[5];
	   Font Impact, Scribe, small;
	   timer *inv_ani;
	   
     public:
	   bool show; // if true, show inventory
	   inventory();
	   ~inventory();
	   void UsePotion();
	   void GetItem(int type, int ID);
	   void Draw();
	   void MouseFunction(int button, int state , int x, int y);
	   item *Equipped();
	   void Dump(std::ofstream &f);
	   void LoadDump(std::ifstream &f);
};

#endif
