#ifndef DungEon
#define DungEon

#include "../entities/monster.h"
#include "fstream"
#include "../core/timer.h"
#include "decor.h"
#include "level.h"
#include <memory>
#include <vector>

// Coordinate spaces used throughout the world system:
//   Map space:    float [0, MAP_W] x [0, MAP_H], tile units, used for collision
//   World space:  map * TILE_SIZE, OpenGL units, used for rendering
//   Screen space: projection of world space, origin top-left

constexpr int CMaxMonsters = 9;

class Dungeon {
  private:
	static constexpr int kMapWidth = LEVEL_WIDTH;
	static constexpr int kMapHeight = LEVEL_HEIGHT;
	static constexpr int kMapCellCount = LEVEL_CELL_COUNT;
	Tint map[kMapCellCount];
	DecorCell decor[kMapCellCount];
	DecalCell decal[kMapCellCount];
	bool torch[kMapCellCount] = {};
	LadderCell ladder[kMapCellCount];
	float mapX, mapY;
	int texC, *Tex;
	bool IsInBounds(int col, int row) const;
	int MapIndex(int col, int row) const;
	Tint MapAt(int col, int row) const;
	void SetMapBAtPlayer(int value);
	void SyncMonsterFromToken(int index);
	void SyncTokenFromMonster(int index, bool includePosition);
	void UpdateMovementState();
	void UpdateMonsters();
	void DrawMonsterTile(int i, int j);
	void DrawTreasureTile(int i, int j);
	void DrawTrapTile(int i, int j, bool isDeathTrap);
	void drawDecorTile(int i, int j);
	void drawDecalTile(int i, int j);
	void scatterDecals(uint32_t seed);
	void scatterTorches(uint32_t seed);
	void drawTorchTile(int i, int j);
	void scatterLadders(uint32_t seed);
	void drawLadderTile(int i, int j);
	struct FlameSource;
	int flamesAt(int i, int j, FlameSource* out) const;
	void addLights();
	void drawFires();
	void DrawSegment(int type, int leftWallType, int rightWallType, int upWallType, int downWallType);
	void renderCartoonTile(int type, int left, int right, int up, int down);
	void renderFlatTile(int type, int left, int right, int up, int down);
	Tint Map(float x, float y) const;
	void InitializeMonsterSlot(int index, int i, int j);
	// Keys, gates, levers and rock falls (dungeon_mechanisms.cpp).
	struct Motion {
		int cell;	 // map index
		int startMs; // GameClock time the motion began
	};
	int keysHeld = 0;				 // bit (colour - 1) per key picked up on this level
	std::vector<Motion> openingGates; // c = 2 while in here, then 1 (open)
	std::vector<Motion> fallingRocks; // c = 2 while in here, then 1 (fallen)
	int lockedHintMs = -1000000;	 // last "the gate is locked" message, to keep it from repeating every step
	void resetMechanisms();
	void updateMechanisms();
	void startOpeningGate(int cell);
	void openGates(int colour);
	void bumpGate(int col, int row); // the player walked into a closed gate
	void updateRocks();
	void drawKeyTile(int i, int j);
	void drawGateTile(int i, int j);
	void drawLeverTile(int i, int j);
	void drawRockFallTile(int i, int j);
	void drawMechanismEffects(); // dust, after the opaque scene
	monsterToken m[CMaxMonsters]; // vienu metu tik 9 monstrai, nes lagin
	bool mL;
	int shaderTexture[1];
	VECTOR lightAngle;
	std::unique_ptr<timer> aniT;
	float plasma = 0.f;
	float qRot = 0.f;

  public:
	Dungeon();
	~Dungeon();
	bool Load(const char* filename);
	void LoadGrid(const LevelGrid& grid, const char* levelName); // levelName seeds the decorations
	// Level `number` of the campaign (campaign.h): a file, or generated from runSeed.
	bool LoadCampaignLevel(int number, uint32_t runSeed);
	void Update();
	void Draw();
	void Move(float dirX, float dirY, bool jump = 0);
	// On a ladder, within reach of it and off the floor: the player hangs on it (climb clip, back to the camera).
	// Walking into a ladder cell from the side keeps the walk / idle clip until climbing pulls the player over.
	[[nodiscard]] bool PlayerOnLadder() const;
	// Climb clip phase: one cycle per tile climbed, running on (at half rate) while moving sideways on the ladder.
	[[nodiscard]] float ClimbPhase() const;
	int Type(float x, float y);
	void getC(float& outX, float& outY);
	void GetAttack(int damage, int attackRange); // Redirects players attack to the nearest monster if in range
	void GetPickUp();							 // not the car... just take an item away
	bool SpawnMonster(int i, int j);
	void GetRiddle();
	bool PullLever(); // interact on a lever cell; false if there is none
	[[nodiscard]] int KeysHeld() const { return keysHeld; }
	void Dump(std::ofstream& f);
	bool LoadDump(std::ifstream& f);
	void scatterDecorations(const char* levelName); // props and decals, seeded by the level's file name
};

#endif
