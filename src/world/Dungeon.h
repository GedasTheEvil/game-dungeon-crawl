#ifndef DungEon
#define DungEon

#include "../entities/monster.h"
#include "fstream"
#include "../core/timer.h"
#include <memory>

enum DungeonTileType {
	Wall = 0,
	Empty = 1,
	Door = 2,
	Death = 3,
	Monster = 4,
	Spike = 5,
	Ladder = 6,
	Area3D = 7,
	Treasure = 8,
	Ankh = 9,
};

enum GateType {
	GateEntrance = 1,
	GateExit = 2,
	GateRiddle = 3,
	GateEmpty = 4,
};

constexpr int CMaxMonsters = 9;

struct Tint {
	int a;
	int b;
	int c;
};

class Dungeon {
  private:
	static constexpr int kMapWidth = 40;
	static constexpr int kMapHeight = 47;
	static constexpr int kMapCellCount = kMapWidth * kMapHeight + 1;
	Tint map[kMapCellCount];
	float x, y;
	int texC, *Tex;
	bool IsInBounds(int mapX, int mapY) const;
	int MapIndex(int mapX, int mapY) const;
	Tint MapAt(int mapX, int mapY) const;
	void SetMapBAtPlayer(int value);
	void SyncMonsterFromToken(int index);
	void SyncTokenFromMonster(int index, bool includePosition);
	void UpdateMovementState();
	void UpdateMonsters();
	void DrawMonsterTile(int i, int j);
	void DrawTreasureTile(int i, int j);
	void DrawTrapTile(int i, int j, bool isDeathTrap);
	void DrawSegment(int seg, int l, int r, int u, int d);
	Tint Map(float x, float y) const;
	void InitializeMonsterSlot(int index, int i, int j);
	monsterToken m[CMaxMonsters]; // vienu metu tik 9 monstrai, nes lagin
	bool mL;
	int shaderTexture[1];
	VECTOR lightAngle;
	std::unique_ptr<timer> aniT;

  public:
	Dungeon();
	~Dungeon();
	bool Load(const char* filename);
	void Update();
	void Draw();
	void Move(float dirX, float dirY, bool jump = 0);
	int Type(float x, float y);
	void getC(float& x, float& y);
	void GetAttack(int dmg, int range); // Redirects players attack to the nearest monster if in range
	void GetPickUp();					// not the car... just take an item away
	bool SpawnMonster(int i, int j);
	void GetRiddle();
	void Dump(std::ofstream& f);
	bool LoadDump(std::ifstream& f);
};

#endif
