#ifndef DungEon
#define DungEon

#include "monster.h"
#include "fstream"
#include "timer.h"

     //dungeon segment types:
     #define Wall     0
     #define Empty    1
     #define Door     2
     #define Death    3
     #define Monster  4
     #define Spike    5
     #define Ladder   6
     #define Area3D   7
     #define Treasure 8
     #define Ankh     9
     
     #define GateEntrance 1
     #define GateExit     2
     #define GateRiddle   3
     #define GateEmpty    4

#define CMaxMonsters 9

struct Tint 
{
     int a;
     int b;
     int c;
};

class Dungeon
{
     private:
          Tint map[1881];
          float x, y;
          int texC, *Tex;
          void DrawSegment(int seg,int l, int r, int u, int d);
	   Tint Map(float x,float y);
	   monsterToken m[CMaxMonsters]; // vienu metu tik 9 monstrai, nes lagin
	   bool mL;
	   int	shaderTexture[1];
	   VECTOR lightAngle;
	   timer *aniT;
	   
     public:
          Dungeon();
	   ~Dungeon();
          bool Load(const char *filename);
          void Draw();
          void Move(float dirX, float dirY, bool jump=0);
          int Type(float x,float y);
	   void getC(float &x, float &y);
	   void GetAttack(int dmg, int range);// Redirects players attack to the nearest monster if in range
	   void GetPickUp(); // not the car... just take an item away
	   bool SpawnMonster(int i, int j);
	   void GetRiddle();
	   void Dump(std::ofstream &f);
	   bool LoadDump(std::ifstream &f);
	   
};

#endif
