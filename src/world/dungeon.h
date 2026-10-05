#ifndef DUNGEON_H
#define DUNGEON_H
#include "../entities/monster.h"
#include "fstream"
#include "../core/timer.h"
#include "../core/gameplay_config.h"
#include "decor.h"
#include "level.h"
#include "view_window.h"
#include "sim_links.h"
#include <memory>
#include <vector>

// Coordinate spaces used throughout the world system:
//   Map space:    float [0, MAP_W] x [0, MAP_H], tile units, used for collision
//   World space:  map * TILE_SIZE, OpenGL units, used for rendering
//   Screen space: projection of world space, origin top-left

constexpr int MAX_MONSTERS = 32; // live monster slots; the campaign's busiest level has 15

// What the hitbox debug view (F3, scenario 'hitboxes on') shows of the weapon: its reach in tiles, measured from the
// player's box edge (melee) or the centre (the bow).
struct HitboxView {
	float reach = 0.f;
	bool fromEdge = true;
	int facing = 1; // the player looks -1 left, +1 right
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
	SimLinks sim; // the player, journal, assets, ... the world was given (Link)
	float mapX, mapY;
	int levelNumber = 1; // the campaign level loaded (campaign.h); a level loaded by path keeps the last number
	bool won = false;	 // the ankh was taken
	// Solid rock for the drawing: a wall, or outside the level.
	[[nodiscard]] bool isRock(int col, int row) const { return !IsInBounds(col, row) || isWall(MapAt(col, row)); }
	[[nodiscard]] ViewWindow view() const { return {static_cast<int>(mapX) - 3, static_cast<int>(mapY) - 3}; }
	bool IsInBounds(int col, int row) const;
	int MapIndex(int col, int row) const;
	Tile MapAt(int col, int row) const;
	void SetMapBAtPlayer(int value);
	void resetPlayerMotion(); // the player was placed on a level: no jump or fall carries over
	void exploreAroundPlayer();
	void UpdateMovementState();
	void UpdateMonsters();
	// A walker can't step into (col, row): a wall, no floor under it (a pit or a drop), or a trap (spikes, a death
	// trap, a rock fall not yet fallen) unless it is reckless (Courage).
	[[nodiscard]] bool walkerBlocked(int col, int row, bool reckless = false) const;
	// The cell a walk-jumper lands on when (col, row) blocks it walking in direction dir: the first one past a gap of
	// up to MONSTER_JUMP_MAX_GAP pits and traps it can walk on. -1: no such cell (a wall, or the gap is too wide).
	[[nodiscard]] int leapLanding(int col, int row, int dir) const;
	[[nodiscard]] float leapTarget(const Monster& mon, int land, int dir) const; // map x of the landing, see Jump
	void clearMonsters(); // a level or save was loaded: the old level's monsters and arrows are gone
	[[nodiscard]] MonsterLinks monsterLinks() const { return {sim.player, sim.journal, sim.events, levelNumber}; }
	// The player's weapon, arrow or a scenario's hit on mon: the journal learns how the weapon's main type works on it
	// (mix; nullptr: untyped), and a kill is rewarded (rewardKill).
	void playerHit(Monster& mon, int dmg, const DamageMix* mix);
	void rewardKill(Monster& mon);			  // the player killed it: journal, XP, maybe a weapon chest
	void DrawMonsters(const CellRect& drawn); // at their actual position, not their spawn tile
	[[nodiscard]] bool inView(const Monster& mon) const;
	void noteSeenMonsters(); // the monsters on screen go in the journal
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
	void addLights(const CellRect& drawn);
	void drawFires(const CellRect& drawn);
	void drawCellSurfaces(int i, int j); // the rock face of a solid cell, the walls, floor and ceiling of an open one
	CellRect drawnCells;				 // the cells the last Draw drew, for DrawWater
	void pushLevelFrame() const;		 // Draw's frame: the view's first cell at the origin, scrolled with the player
	void drawWaterCell(int i, int j, float x, float y);
	// World units to draw something standing at map x on row's floor down into a water basin: WATER_BASIN_DEPTH in
	// half water, growing over WATER_SINK_RAMP from a dry edge, 0 elsewhere.
	[[nodiscard]] float waterSink(float x, int row) const;
	[[nodiscard]] bool dryOpen(int col, int row) const; // walkable and not water: a basin's edge
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
	int waterHintMs = -1000000;		  // last "too deep to jump" message
	int wadeSoundMs = -1000000;		  // last splashing step
	void resetMechanisms();
	void updateMechanisms();
	void startOpeningGate(int cell);
	void openGates(int colour);
	void bumpGate(int col, int row); // the player walked into a closed gate
	void startRockFall(int cell);	 // an armed rock fall cell was stepped into: the rumble starts
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
		DamageMix mix;
		int startMs;	  // GameClock time of the shot
		float t = 0.f;	  // seconds flown so far
		int stuckMs = -1; // GameClock time it hit a wall or the floor; < 0: flying
		[[nodiscard]] float X() const { return x0 + vx * t; }
		[[nodiscard]] float Y() const;
	};
	std::vector<Arrow> arrows;
	void updateArrows();
	void dropChest(const Monster& mon, ItemKind weapon); // RollKillDrop
	// Spike and death trap tiles hurt the player and the monsters standing in them (TrapHurt).
	TrapHurt trapHurt; // the player's
	// (x, y) in map units is in a spike or death trap's hitbox: TRAP_HITBOX_X/Y_SCALE x the trap's scale round the
	// tile's bottom centre.
	[[nodiscard]] bool inTrap(float x, float y) const;
	void updateTraps();
	void drawArrows(); // with the frame origin of DrawMonsters
	// The near edge (at mid height) of the nearest living monster ahead (dir -1 / +1) within range tiles of (x, y) the
	// bow can reach, or false.
	bool aimTarget(float x, float y, int dir, float range, float& outX, float& outY) const;
	Timer portalTimer{50}; // steps the portal texture scroll
	float portalScroll = 0.f;
	float riddleMarkYaw = 0.f; // the spinning question mark over a riddle gate
	float treasureSpin = 0.f;  // degrees: the item turning over a treasure chest
	// The monster tiles in the gameplay window (ViewWindow, 10 x 6 cells round the player) spawn their monsters.
	void spawnInView();
	void updateAnimations(); // the portal scroll, the riddle mark, the treasure items

  public:
	Dungeon();
	~Dungeon();
	void Link(const SimLinks& links) { sim = links; } // once, before the first level
	bool Load(const char* filename);
	void LoadGrid(const LevelGrid& grid, const char* levelName); // levelName seeds the decorations
	// Level `number` of the campaign (campaign.h).
	bool LoadCampaignLevel(int number);
	[[nodiscard]] int LevelNumber() const { return levelNumber; }
	void SetLevelNumber(int number) { levelNumber = number; } // a save game was loaded
	[[nodiscard]] bool Won() const { return won; }
	void ClearWin() { won = false; } // a new game or a scenario level
	void Update();
	void AnimateMonsters(); // once a tick, after Update: every active monster (Monster::Animate)
	void Draw(const HitboxView* hitboxes = nullptr); // hitboxes: the debug view, nullptr when off
	// The water over everything in it (the player too), see-through: after the player, in Draw's frame.
	void DrawWater();
	bool Move(float dirX, float dirY); // false: blocked, the player did not move
	// Standing in half water (not in the air): the walk slows to WADE_SPEED_FACTOR, no jump.
	[[nodiscard]] bool PlayerWading() const;
	[[nodiscard]] float PlayerWalkFactor() const { return PlayerWading() ? WADE_SPEED_FACTOR : 1.f; }
	// False while wading: then the player is told why (at most every WATER_JUMP_HINT_MS).
	bool JumpAllowed();
	// World units the player is drawn down into a water basin (waterSink, less while above the floor: a fall, a jump).
	[[nodiscard]] float PlayerSink() const;
	// On a ladder, within reach of it and off the floor: the player hangs on it (climb clip, back to the camera).
	// Walking into a ladder cell from the side keeps the walk / idle clip until climbing pulls the player over.
	[[nodiscard]] bool PlayerOnLadder() const;
	// Climb clip phase: one cycle per tile climbed, running on (at half rate) while moving sideways on the ladder.
	[[nodiscard]] float ClimbPhase() const;
	int Type(float x, float y);
	void getC(float& outX, float& outY);
	// The player's melee attack: hits the nearest monster in range ahead (dir -1 / +1). False: nothing in reach.
	bool AttackNearest(int damage, const DamageMix& mix, float reach, int dir); // reach in tiles (Item::Reach)
	// The bow fires: an arrow leaves the bow, height above the player's feet in tiles, facing dir (-1 / +1),
	// aimed at a monster up to aimRange tiles ahead.
	void ShootArrow(int damage, const DamageMix& mix, int dir, float height, float aimRange);
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
	[[nodiscard]] int ChestCount() const;			// treasure chests not opened yet
	[[nodiscard]] int CoffinCount() const;			// coffins standing on the level (decorations)
	void HurtBoss(int dmg);							// scenario tests: the boss in play takes a hit, as from the player
	// A minion's kill: 1 XP while its boss lives (no farming), half its type's xp after the boss died.
	[[nodiscard]] int MinionXP(int xp) const { return bossFight.slot >= 0 ? 1 : xp / 2; }
	// Draft map: the cells within EXPLORE_RADIUS of every tile the player stood on.
	static constexpr int EXPLORE_RADIUS = 1; // cells to each side: a 3 x 3 square
	[[nodiscard]] bool Explored(int col, int row) const { return IsInBounds(col, row) && explored[MapIndex(col, row)]; }
	[[nodiscard]] Tile Cell(int col, int row) const { return MapAt(col, row); }
	void Dump(std::ofstream& f);
	bool LoadDump(std::ifstream& f);
	void scatterDecorations(const char* levelName);	   // props and decals, seeded by the level's file name
	[[nodiscard]] bool bossCoffin(int i, int j) const; // a coffin for the boss's minions stands there
};

#endif
