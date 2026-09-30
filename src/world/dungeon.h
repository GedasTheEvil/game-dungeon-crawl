#ifndef DUNGEON_H
#define DUNGEON_H
#include "../entities/monster.h"
#include "fstream"
#include "../core/timer.h"
#include "../core/gameplay_config.h"
#include "decor.h"
#include "level.h"
#include <memory>
#include <vector>

// Coordinate spaces used throughout the world system:
//   Map space:    float [0, MAP_W] x [0, MAP_H], tile units, used for collision
//   World space:  map * TILE_SIZE, OpenGL units, used for rendering
//   Screen space: projection of world space, origin top-left

constexpr int MAX_MONSTERS = 32; // live monster slots; the campaign's busiest level has 15

// The part of the level drawn round the player: WIDTH x HEIGHT cells from (firstCol(), originRow). Dungeon::Draw's
// frame puts cell (originCol, originRow) at the origin; the column left of it is drawn too, for the view's edge.
struct ViewWindow {
	static constexpr int WIDTH = 10;
	static constexpr int HEIGHT = 6;
	int originCol = 0;
	int originRow = 0;
	[[nodiscard]] int firstCol() const { return originCol - 1; }
	[[nodiscard]] bool contains(float x, int row) const {
		return row >= originRow && row < originRow + HEIGHT && x >= static_cast<float>(firstCol()) &&
			   x < static_cast<float>(firstCol() + WIDTH);
	}
};

// What the hitbox debug view (F3, scenario 'hitboxes on') shows of the weapon: its reach in tiles, measured from the
// player's box edge (melee) or the centre (the bow).
struct HitboxView {
	float reach = 0.f;
	bool fromEdge = true;
};

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
	// Solid rock for the drawing: a wall, or outside the level.
	[[nodiscard]] bool isRock(int col, int row) const { return !IsInBounds(col, row) || MapAt(col, row).type == Wall; }
	[[nodiscard]] ViewWindow view() const { return {static_cast<int>(mapX) - 3, static_cast<int>(mapY) - 3}; }
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
	[[nodiscard]] float leapTarget(const Monster& mon, int land, int dir) const; // map x of the landing, see Jump
	void clearMonsters(); // a level or save was loaded: the old level's monsters and arrows are gone
	void DrawMonsters();  // at their actual position, not their spawn tile
	[[nodiscard]] bool inView(const Monster& mon) const;
	void drawHitboxes(const HitboxView& weapon);
	void DrawTreasureTile(int i, int j);
	void DrawTrapTile(bool isDeathTrap);
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
	int levelKeys = 0;				  // bit (colour - 1) per key the level has, picked up or not
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
	void drawTeleporterTile();			// at the cell origin: the columns face the camera, plasma between them
	void drawTileContent(int i, int j); // what stands in the cell, by tile type
	void drawAnkhTile();
	void drawDoorTile(int i, int j); // a sphinx gate: entrance, exit, riddle or empty
	// The scrolling plasma of the gates, over the quad v (its corners take the texture's (0, 0), (0, 1), (1, 1), (1,
	// 0)).
	void drawPortal(const float normal[3], const float v[4][3]) const;
	void drawRockFallTile(int i, int j);
	void drawMechanismEffects(); // dust, after the opaque scene
	Monster monsters[MAX_MONSTERS];
	[[nodiscard]] Monster* freeMonsterSlot(); // an empty slot, else the slot of a dead monster; null if none
	// The level's boss fight (at most one boss per level). Monsters are not saved: a load starts the fight over.
	struct BossFight {
		int slot = -1;		  // monsters[] index of the boss, -1: none in play
		int summoned = 0;	  // minions summoned this fight, after the first ones
		int nextSummonMs = 0; // GameClock time of the next summon
	} bossFight;
	void startBossFight(int slot); // the boss appeared: its first minions with it
	void updateBoss();			   // summons while it lives; its death opens the boss gates, for good
	bool summonMinion(const Monster& boss);
	void drawSummonEffects(); // sand and dust where minions came out, after the opaque scene
	// Arrows (dungeon_arrows.cpp): a parabola from the launch point, x0 + vx t, y0 + vy t - g t^2 / 2 in map units.
	struct Arrow {
		float x0, y0, vx, vy;
		int damage;
		int startMs;	  // GameClock time of the shot
		float t = 0.f;	  // seconds flown so far
		int stuckMs = -1; // GameClock time it hit a wall or the floor; < 0: flying
		[[nodiscard]] float X() const { return x0 + vx * t; }
		[[nodiscard]] float Y() const;
	};
	std::vector<Arrow> arrows;
	void updateArrows();
	// Spike and death trap tiles hurt the player standing in them (a hitbox of TRAP_HITBOX_X/Y_SCALE x the trap's
	// scale round the tile's bottom centre), every TRAP_HURT_INTERVAL_MS, however many traps: one timer for the player.
	// The damage grows while the player stays in (TRAP_DAMAGE_RAMP_HITS); a gap of TRAP_STREAK_RESET_MS resets it.
	struct TrapHurt {
		Timer timer{TRAP_HURT_INTERVAL_MS};
		int streak = 0;
		int lastHitMs = 0;
	} trapHurt;
	void updateTraps();
	void drawArrows(); // with the frame origin of DrawMonsters
	// The near edge (at mid height) of the nearest living monster ahead (dir -1 / +1) within range tiles of (x, y) the
	// bow can reach, or false.
	bool aimTarget(float x, float y, int dir, float range, float& outX, float& outY) const;
	Timer portalTimer{50}; // steps the portal texture scroll
	float portalScroll = 0.f;
	float riddleMarkYaw = 0.f; // the spinning question mark over a riddle gate
	float treasureSpin = 0.f;  // degrees: the item turning over a treasure chest
	// The monster tiles in the drawn window (10 x 6 cells round the player) spawn their monsters (SpawnMonster).
	void spawnInView();
	void updateAnimations(); // the portal scroll, the riddle mark, the treasure items

  public:
	Dungeon();
	~Dungeon();
	bool Load(const char* filename);
	void LoadGrid(const LevelGrid& grid, const char* levelName); // levelName seeds the decorations
	// Level `number` of the campaign (campaign.h).
	bool LoadCampaignLevel(int number);
	void Update();
	void AnimateMonsters(); // once a tick, after Update: every active monster (Monster::Animate)
	void Draw(const HitboxView* hitboxes = nullptr); // hitboxes: the debug view, nullptr when off
	void Move(float dirX, float dirY);
	// On a ladder, within reach of it and off the floor: the player hangs on it (climb clip, back to the camera).
	// Walking into a ladder cell from the side keeps the walk / idle clip until climbing pulls the player over.
	[[nodiscard]] bool PlayerOnLadder() const;
	// Climb clip phase: one cycle per tile climbed, running on (at half rate) while moving sideways on the ladder.
	[[nodiscard]] float ClimbPhase() const;
	int Type(float x, float y);
	void getC(float& outX, float& outY);
	// The player's melee attack: hits the nearest monster in range ahead (dir -1 / +1). False: nothing in reach.
	bool AttackNearest(int damage, float reach, int dir); // reach in tiles (Item::Reach)
	// The bow fires: an arrow leaves the bow, height above the player's feet in tiles, facing dir (-1 / +1),
	// aimed at a monster up to aimRange tiles ahead.
	void ShootArrow(int damage, int dir, float height, float aimRange);
	void PickUp(); // not the car... just take an item away
	bool SpawnMonster(int i, int j);
	void Interact();
	void Teleport();  // on a teleporter: jump to its partner
	bool PullLever(); // interact on a lever cell; false if there is none
	[[nodiscard]] int KeysHeld() const { return keysHeld; }
	[[nodiscard]] int LevelKeys() const { return levelKeys; } // the HUD's key sockets
	[[nodiscard]] int MonsterBarsShown() const;				  // living monsters that show their health bar
	// The boss in a fight (alive and alerted), for the HUD bar; null if none.
	[[nodiscard]] const Monster* Boss() const;
	[[nodiscard]] int BossHealth() const; // of the boss in play (alerted or not), 0 if none
	[[nodiscard]] int LivingMinions() const;
	[[nodiscard]] int NearestMonsterHealth() const; // of the living monster nearest the player, 0 if none
	void HurtBoss(int dmg);							// scenario tests: the boss in play takes a hit, as from the player
	// A minion's kill: 1 XP while its boss lives (no farming), half its type's xp after the boss died.
	[[nodiscard]] int MinionXP(int xp) const { return bossFight.slot >= 0 ? 1 : xp / 2; }
	// Draft map: the cells within EXPLORE_RADIUS of every tile the player stood on.
	static constexpr int EXPLORE_RADIUS = 1; // cells to each side: a 3 x 3 square
	[[nodiscard]] bool Explored(int col, int row) const { return IsInBounds(col, row) && explored[MapIndex(col, row)]; }
	[[nodiscard]] Tile Cell(int col, int row) const { return MapAt(col, row); }
	void Dump(std::ofstream& f);
	bool LoadDump(std::ifstream& f);
	void scatterDecorations(const char* levelName); // props and decals, seeded by the level's file name
};

#endif
