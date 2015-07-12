#ifndef StatsH
#define StatsH

#include "monster.h"
#include "item.h"
#include "inventory.h"
#include "fstream"

class stats
{
     private:
	   int level;
	   double XP;
	   bool AdvanceLevel();
	   
	   int Armor;
	   int MaxHP;
	   int HP;
	   int Might;
	   
	   Font Impact;
	   
	   float realPscale; // player scale
	   float realIscale; // item scale
	   timer *stats_ani;
	   
     public:
	   int Damage();
	   bool show;
	   void GetStronger(int ns = 1);
	   void Draw();
	   void GetXP(int xp);
	   void Heal(int hp_part);
	   stats();
	   ~stats();
	   void GetArmored(int na = 1);
	   void GetHit(int dmg);
	   void MouseFunction(int button, int state , int x, int y);
	   void GetTougher(int hp_part);
	   void Dump(std::ofstream &f);
	   void LoadDump(std::ifstream &f);
};

#endif
