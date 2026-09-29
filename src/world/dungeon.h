#ifndef DUNGEON_H
#define DUNGEON_H
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

constexpr int MAX_MONSTERS = 32; // live monster slots; the campaign's busiest level has 15

class Dungeon {
  private:
	static constexpr int MAP_WIDTH = LEVEL_WIDTH;
	static constexpr int MAP_HEIGHT = LEVEL_HEIGHT;
	static constexpr int MAP_CELL_COUNT = LEVEL_CELL_COUNT;
	Tile map[MAP_CELL_COUNT];
	DecorCell decor[MAP_CELL_COUNT];
	DecalCell decal[MAP_CELL_COUNT];
	SurfaceCell surface[MAP_CELL_COUNT];
	bool torch[MAP_CELL_COUNT] = {};
	bool explored[MAP_CELL_COUNT] = {}; // cells on the draft map (map_view.h)
	LadderCell ladder[MAP_CELL_COUNT];
	float mapX, mapY;
	bool IsInBounds(int col, int row) const;
	int MapIndex(int col, int row) const;
	Tile MapAt(int col, int row) const;
	void SetMapBAtPlayer(int value);
	void resetPlayerMotion(); // the player was placed on a level: no jump or fall carries over
	void exploreAroundPlayer();
	void UpdateMovementState();
	void UpdateMonsters();
	// A walker can't step into (col, row): a wall, a trap, or no floor under it (a pit or a drop).
	[[nodiscard]] bool walkerBlocked(int col, int row) const;
	// The cell a walk-jumper lands on when (col, row) blocks it walking in direction dir: the first one past a gap of
	// up to MONSTER_JUMP_MAX_GAP pits and traps it can walk on. -1: no such cell (a wall, or the gap is too wide).
	[[nodiscard]] int leapLanding(int col, int row, int dir) const;
	void clearMonsters(); // a level or save was loaded: the old level's monsters are gone
	void DrawMonsters();  // at their actual position, not their spawn tile
	void DrawTreasureTile(int i, int j);
	void DrawTrapTile(int i, int j, bool isDeathTrap);
	void drawDecorTile(int i, int j);
	void drawDecalTile(int i, int j);
	void scatterDecals(uint32_t seed);
	void scatterSurfaces(uint32_t seed);
	void scatterTorches(uint32_t seed);
	void drawTorchTile(int i, int j);
	void scatterLadders(uint32_t seed);
	void drawLadderTile(int i, int j);
	struct FlameSource;
	int flamesAt(int i, int j, FlameSource* out) const;
	void addLights();
	void drawFires();
	void drawCellSurfaces(int i, int j); // the rock face of a solid cell, the walls, floor and ceiling of an open one
	Tile Map(float x, float y) const;
	// Keys, gates, levers and rock falls (dungeon_mechanisms.cpp).
	struct Motion {
		int cell;	 // map index
		int startMs; // GameClock time the motion began
	};
	int keysHeld = 0;				  // bit (colour - 1) per key picked up on this level
	std::vector<Motion> openingGates; // c = 2 while in here, then 1 (open)
	std::vector<Motion> fallingRocks; // c = 2 while in here, then 1 (fallen)
	int lockedHintMs = -1000000;	  // last "the gate is locked" message, to keep it from repeating every step
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
	Monster monsters[MAX_MONSTERS];
	Timer portalTimer{50}; // steps the portal texture scroll
	float portalScroll = 0.f;
	float riddleMarkYaw = 0.f; // the spinning question mark over a riddle gate

  public:
	Dungeon();
	~Dungeon();
	bool Load(const char* filename);
	void LoadGrid(const LevelGrid& grid, const char* levelName); // levelName seeds the decorations
	// Level `number` of the campaign (campaign.h).
	bool LoadCampaignLevel(int number);
	void Update();
	void Draw();
	void Move(float dirX, float dirY, bool jump = false);
	// On a ladder, within reach of it and off the floor: the player hangs on it (climb clip, back to the camera).
	// Walking into a ladder cell from the side keeps the walk / idle clip until climbing pulls the player over.
	[[nodiscard]] bool PlayerOnLadder() const;
	// Climb clip phase: one cycle per tile climbed, running on (at half rate) while moving sideways on the ladder.
	[[nodiscard]] float ClimbPhase() const;
	int Type(float x, float y);
	void getC(float& outX, float& outY);
	void AttackNearest(int damage, int attackRange); // Redirects players attack to the nearest monster if in range
	void PickUp();									 // not the car... just take an item away
	bool SpawnMonster(int i, int j);
	void Interact();
	bool PullLever(); // interact on a lever cell; false if there is none
	[[nodiscard]] int KeysHeld() const { return keysHeld; }
	[[nodiscard]] int MonsterBarsShown() const; // living monsters that show their health bar
	// Draft map: the cells within EXPLORE_RADIUS of every tile the player stood on.
	static constexpr int EXPLORE_RADIUS = 1; // cells to each side: a 3 x 3 square
	[[nodiscard]] bool Explored(int col, int row) const { return IsInBounds(col, row) && explored[MapIndex(col, row)]; }
	[[nodiscard]] Tile Cell(int col, int row) const { return MapAt(col, row); }
	void Dump(std::ofstream& f);
	bool LoadDump(std::ifstream& f);
	void scatterDecorations(const char* levelName); // props and decals, seeded by the level's file name
};

#endif
