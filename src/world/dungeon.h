#ifndef DUNGEON_H
#define DUNGEON_H
#include "../entities/monster.h"
#include <fstream>
#include "../core/timer.h"
#include "../core/gameplay_config.h"
#include "decor_scatter.h"
#include "dungeon_darts.h"
#include "interaction.h"
#include "level.h"
#include "climb_path.h"
#include "view_window.h"
#include "sim_links.h"
#include <memory>
#include <vector>

// Coordinate spaces used throughout the world system:
//   Map space:    float [0, MAP_W] x [0, MAP_H], tile units, used for collision
//   World space:  map * TILE_SIZE, OpenGL units, used for rendering
//   Screen space: projection of world space, origin top-left

// Live monster slots; the campaign's busiest level has 15. No design limit: raise it at will.
constexpr int MAX_MONSTERS = 32;

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
	DecorLayout decoration; // props, decals, torches, ladders and surfaces (scatterDecorations)
	bool explored[MAP_CELL_COUNT] = {}; // cells on the draft map (map_view.h)
	SimLinks sim; // the player, journal, assets, ... the world was given (Link)
	float mapX, mapY;
	int levelNumber = 1; // the campaign level loaded (campaign.h); a level loaded by path keeps the last number
	bool won = false; // the ankh was taken
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
	bool PickUp(); // not the car... just take an item away; false if there is none
	// Treasure containers (plan treasure-containers, containers.h): what was done to the one in each cell.
	struct ContainerCell {
		bool opened = false; // its clip started at openedMs: the item shows once it has played
		bool emptied = false; // the item was taken: the open container stays
		bool dropped = false; // a kill's or a dead mimic's chest: DROPPED_LOOK, open from the start
		int openedMs = 0; // GameClock
	};
	ContainerCell containers[MAP_CELL_COUNT];
	static constexpr int CONTAINER_OPENED_LONG_AGO_MS = 1000000; // a loaded open container's clip has played
	static char containerChar(const ContainerCell& c); // in a save (dungeon_io.cpp)
	static ContainerCell containerFromChar(char c);
	void openContainer(); // Interact Open on a closed treasure: its clip starts
	// A chest a kill or a dead mimic leaves on cell (map index): open, its item showing.
	void placeDroppedChest(int cell, ItemFileId item);
	[[nodiscard]] bool containerShows(int cell) const; // opened and its clip played: the item shows
	// 0 closed .. 1 open, the cell's clip.
	[[nodiscard]] float containerOpening(int cell) const;
	[[nodiscard]] TreasureLook treasureLook(int cell) const;
	bool PullLever(); // interact on a lever cell; false if there is none
	bool readInscription(); // interact on an inscription: into the journal; false if there is none
	// Interact by a brazier with a bow held (InteractVerb Light): the next FIRE_ARROW_SHOTS arrows carry fire (plan
	// fire-damage).
	void lightArrows();
	void useGate(); // the exit, a teleporter, a riddle gate, the ankh
	// The slot of the mimic lying in ambush in the player's cell, looking like a chest (plan interact-prompt); -1 if
	// none.
	[[nodiscard]] int ambushHere() const;
	// UpdateMonsters' part per locomotion, each in its file (dungeon_walkers.cpp, dungeon_flight.cpp,
	// dungeon_burrow.cpp, dungeon_charge.cpp). updateBurrower, updateCharger: true while it burrows or charges (nothing
	// else moves it).
	void updateWalker(Monster& mon);
	void updateFlyer(Monster& mon);
	bool updateBurrower(Monster& mon);
	bool updateCharger(Monster& mon);
	// Walkers (dungeon_walkers.cpp). A walker can't step into (col, row): a wall, no floor under it (a pit or a drop),
	// or a trap (spikes, a death trap, a rock fall not yet fallen) unless it is reckless (Courage).
	[[nodiscard]] bool walkerBlocked(int col, int row, bool reckless = false) const;
	// The cell a walk-jumper lands on when (col, row) blocks it walking in direction dir: the first one past a gap of
	// up to MONSTER_JUMP_MAX_GAP pits and traps it can walk on. -1: no such cell (a wall, or the gap is too wide).
	[[nodiscard]] int leapLanding(int col, int row, int dir) const;
	[[nodiscard]] float leapTarget(const Monster& mon, int land, int dir) const; // map x of the landing, see Jump
	// A walker on the player's row can get to them that way (dir -1 / +1): walking, a walk-jumper leaping gaps.
	[[nodiscard]] bool walkerReaches(const Monster& mon, int dir) const;
	// A coward that cannot reach the player (Monster::flees): runs away along its row, out of the bow's range.
	void fleeFrom(Monster& mon, int dir);
	// A climber that has seen the player follows its path to them across floors (ClimbMap) while they are off its row
	// or out of its reach along it; dir: Monster::attackDirection. False: the walker's rules go on (no path, given up).
	bool followPath(Monster& mon, int dir);
	// Flyers (dungeon_flight.cpp, plan bats-high-ceiling): whether it sees the player, the row it heads for and the way
	// there.
	[[nodiscard]] FlightGoal flightGoal(const Monster& mon) const;
	// A flyer's way from (col, row) to (toCol, toRow) through open cells, never up or down into half water; boxed: only
	// inside the rectangle of the two cells. The column of its next step up or down; -1: no way (or the same row).
	[[nodiscard]] int flightShaft(int col, int row, int toCol, int toRow, bool boxed) const;
	[[nodiscard]] int climbEnd(int col, int row) const; // the row a climb from (col, row) runs to, along climbPath
	ClimbMap climbPath; // the last path followPath built
	void clearMonsters(); // a level or save was loaded: the old level's monsters, missiles and shots are gone
	[[nodiscard]] MonsterLinks monsterLinks() const {
		return {sim.player, sim.journal, sim.events, levelNumber, &sim.random->gameplay};
	}
	// The player's weapon, arrow or a scenario's hit on mon: the journal learns how the weapon's main type works on it
	// (mix; nullptr: untyped), and a kill is rewarded (rewardKill).
	void playerHit(Monster& mon, int dmg, const DamageMix* mix);
	void touchPoison(const Monster& mon); // the plant's sap, after a melee hit
	void venomHit(Monster& mon); // the venom amulet's roll after a weapon hit
	void rewardKill(Monster& mon); // the player killed it: journal, XP, maybe a weapon chest
	// A killed monster's spawn tile turns into its slain marker (slainObject): it does not come back, not after a load
	// nor when its slot is reused. Not a minion's (it has no tile), not a mimic's (its tile turns into its chest).
	void markSlain(const Monster& mon);
	void DrawMonsters(const CellRect& drawn); // at their actual position, not their spawn tile
	[[nodiscard]] bool inView(const Monster& mon) const;
	void noteSeenMonsters(); // the monsters on screen go in the journal
	void drawHitboxes(const HitboxView& weapon);
	void DrawTreasureTile(int i, int j); // the container, open or not, and the item once it shows
	void drawContainerEffects(); // the dust of a jar breaking, after the opaque scene
	void DrawTrapTile(bool isDeathTrap);
	void drawDecorTile(int i, int j);
	void drawDecalTile(int i, int j);
	void drawTorchTile(int i, int j);
	void drawLadderTile(int i, int j);
	void addLights(const CellRect& drawn);
	void drawFires(const CellRect& drawn);
	void drawCellSurfaces(int i, int j); // the rock face of a solid cell, the walls, floor and ceiling of an open one
	CellRect drawnCells; // the cells the last Draw drew, for DrawWater
	void pushLevelFrame() const; // Draw's frame: the view's first cell at the origin, scrolled with the player
	void drawWaterCell(int i, int j, float x, float y);
	[[nodiscard]] bool dryOpen(int col, int row) const; // walkable and not water: a basin's edge
	[[nodiscard]] bool deepAt(int col, int row) const;
	// A deep water cell's water inside its rock slabs, in world units from the cell's corner (WATER_SLAB).
	struct WaterBox {
		float left, right, bottom;
	};
	[[nodiscard]] WaterBox deepWaterBox(int col, int row) const;
	void drawRockStrip(int i, int j, float x0, float x1, float y0, float y1) const;
	Tile Map(float x, float y) const;
	// Keys, gates, levers and rock falls (dungeon_mechanisms.cpp, drawn in dungeon_render_mechanisms.cpp).
	struct Motion {
		int cell; // map index
		int startMs; // GameClock time the motion began
	};
	int keysHeld = 0; // bit (colour - 1) per key picked up on this level
	int levelKeys = 0; // bit (colour - 1) per key the level has, picked up or not
	std::vector<Motion> openingGates; // c = 2 while in here, then 1 (open)
	std::vector<Motion> fallingRocks; // c = 2 while in here, then 1 (fallen)
	int lockedHintMs = -1000000; // last "the gate is locked" message, to keep it from repeating every step
	int waterHintMs = -1000000; // last "too deep to jump" message
	int wadeSoundMs = -1000000; // last splashing step
	void resetMechanisms();
	void updateMechanisms();
	void startOpeningGate(int cell);
	void openGates(int colour);
	void bumpGate(int col, int row); // the player walked into a closed gate
	void startRockFall(int cell); // an armed rock fall cell was stepped into: the rumble starts
	void updateRocks();
	void drawKeyTile(int i, int j);
	void drawGateTile(int i, int j);
	void drawLeverTile(int i, int j);
	void drawTeleporterTile(); // at the cell origin: the columns face the camera, plasma between them
	void drawTileContent(int i, int j); // the fixture on the back wall, then what stands in the cell, by tile type
	void drawFixture(int i, int j);
	void drawAnkhTile();
	void drawDoorTile(int i, int j); // a sphinx gate: entrance, exit, riddle or empty
	// The scrolling plasma of the gates, over the quad v (its corners take the texture's (0, 0), (0, 1), (1, 1), (1,
	// 0)).
	void drawPortal(const float normal[3], const float v[4][3], bool red = false) const;
	void drawRockFallTile(int i, int j);
	void drawMechanismEffects(); // dust, after the opaque scene
	DartTraps dartTraps;
	void updateDartTraps();
	[[nodiscard]] bool plateLoaded(int col, int row) const; // the player or a heavy monster stands on it
	void pressPlate(int col, int row, bool byPlayer);
	void updateDarts();
	void drawDartPlateTile(int i, int j); // the plate and the dart holes above it
	void drawInscriptionTile(int i, int j); // its atlas cell on the back wall
	void drawDarts(); // with the frame origin of DrawMonsters
	// Wall spikes (dungeon_spikes.cpp, plan rhythm-traps): each cell's live state from the clock; coming out, they stab
	// and shove out whoever stands in the cell.
	void updateWallSpikes();
	void stabWallSpikes(int col, int row);
	[[nodiscard]] std::optional<float> outOfSpikes(int col, int row, float centre, float half) const;
	void drawWallSpikesTile(int i, int j); // the slotted panel and the spikes, as far out as the cycle has them
	Monster monsters[MAX_MONSTERS];
	[[nodiscard]] const Monster* nearestMonster() const; // the living one nearest the player; null if none
	[[nodiscard]] Monster* freeMonsterSlot(); // an empty slot, else the slot of a dead monster; null if none
	// The level's boss fight (at most one boss per level). Monsters are not saved: a load starts the fight over.
	struct BossFight {
		int slot = -1; // monsters[] index of the boss, -1: none in play
		int summoned = 0; // minions summoned this fight, after the first ones
		int nextSummonMs = 0; // GameClock time of the next summon
		int firstLeft = 0; // hatchers: first minions still to come (the egg clusters spawn in view after the boss)
		int hatched = 0; // minions of any kind so far: picks the kind (BossRules::smallMinion) and the nest
	} bossFight;
	void startBossFight(int slot); // the boss appeared: its first minions with it
	void updateBoss(); // summons while it lives; its death opens the boss gates, for good
	bool summonMinion(const Monster& boss);
	// Summon::Wall: the wall cell on the player's row the way they look, with a free cell in front of it the minion
	// steps onto, not the player's own; else the same behind them. One on screen first (WALL_SUMMON_SIGHT), then any.
	// False: none (the row ends in no wall).
	bool wallSummonCell(int& outWall, int& outCell, int& outRow) const;
	int playerFacing = -1; // the way the player last walked or turned (Move): -1 left (as the game starts), +1 right
	void drawSummonEffects(); // sand and dust where minions came out, the burrowers' holes, after the opaque scene
	// Where a burrower went down or came up (Monster::Dive): sand thrown up and a dark hole, for BURROW_HOLE_MS.
	struct Hole {
		float x; // map x
		int row;
		int startMs;
	};
	std::vector<Hole> holes;
	void drawHole(float x, float y, float z, float fade) const;
	// Map x a burrower comes up at: BURROW_BEHIND past the player, away from where it is; else as far in front of
	// them. False: neither is floor it can stand on.
	[[nodiscard]] bool burrowTarget(const Monster& mon, float& outX) const;
	// Missiles (dungeon_arrows.cpp): arrows, sling stones, throwing sticks and javelins. A parabola from the launch
	// point, x0 + vx t, y0 + vy t - g t^2 / 2 in map units.
	struct Missile {
		MissileKind kind;
		float x0, y0, vx, vy;
		int damage;
		DamageMix mix;
		int startMs; // GameClock time of the shot
		bool fire = false; // a fire arrow (lightArrows): it flies burning
		float t = 0.f; // seconds flown so far
		int stuckMs = -1; // GameClock time it hit a wall or the floor (or a monster: the stick turns back); < 0: flying
		float endX = 0.f, endY = 0.f; // where it stopped: the throwing stick flies back from there
		[[nodiscard]] float X() const { return x0 + vx * t; }
		[[nodiscard]] float Y() const;
	};
	std::vector<Missile> missiles;
	void updateMissiles();
	// A monster's ranged attack in flight, venom or bolt (dungeon_arrows.cpp): a straight line from its release point,
	// map units.
	struct MonsterShot {
		float x, y, vx, vy;
		const MonsterType* from; // its damage, mix and poison
		Rgb colour; // RangedRules::colour
		int lastMs; // GameClock time of the last update
		float flown = 0.f; // tiles
		int splatMs = -1; // GameClock time it hit a wall or the floor; < 0: flying
		int spawnMs = 0; // GameClock time it was shot: the seed of its flames
	};
	std::vector<MonsterShot> shots;
	void shootAtPlayer(const Monster& mon, float x, float y);
	void updateShots();
	void addVenomLights();
	void addFireLights(); // the fire arrows' and the burning monsters' (plan fire-damage)
	void drawBurning(); // flames on the burning monsters and the fire arrows; drawSummonEffects' frame
	void drawShots(); // with the frame origin of DrawMonsters
	[[nodiscard]] bool clearRow(float fromX, float toX, int row) const; // no solid cell between the two on row

	void dropChest(const Monster& mon, ItemKind weapon); // RollKillDrop
	// Spike and death trap tiles hurt the player and the monsters standing in them (TrapHurt).
	TrapHurt trapHurt; // the player's
	// (x, y) in map units is in a spike or death trap's hitbox: TRAP_HITBOX_X/Y_SCALE x the trap's scale round the
	// tile's bottom centre.
	// The tile type (Spike or Death) it stands in, else nullopt.
	[[nodiscard]] std::optional<int> inTrap(float x, float y) const;
	bool playerVenomed = false; // the death trap's poison took this entry (reset on leaving it)
	void updateTraps();
	void updateMonsterPoison(); // the running poison of each monster; the player's poison kill is theirs
	void updateBurning(); // the player's and each monster's fire (plan fire-damage); a monster's burning kill rewards
	void drawMissiles(); // with the frame origin of DrawMonsters
	// The near edge (at mid height) of the nearest living monster ahead (dir -1 / +1) within range tiles of (x, y) the
	// bow can reach, or false.
	bool aimTarget(float x, float y, int dir, float range, float& outX, float& outY) const;
	// Gate travel: the view fades out (Out), the jump happens under the faded frame, the view fades in (In). The world
	// stands still meanwhile.
	struct GateTravel {
		enum class Phase : unsigned char { None, Out, In };
		Phase phase = Phase::None;
		int startMs = 0;
		bool exit = false; // the exit gate (red), else a teleporter (blue)
	} travel;
	void startTravel(bool exitGate);
	void updateTravel();
	Timer portalTimer{50}; // steps the portal texture scroll
	float portalScroll = 0.f;
	float riddleMarkYaw = 0.f; // the spinning question mark over a riddle gate
	float treasureSpin = 0.f; // degrees: the item turning over a treasure chest
	// The monster tiles in the gameplay window (ViewWindow, 10 x 6 cells round the player) spawn their monsters.
	void spawnInView();
	void updateAnimations(); // the portal scroll, the riddle mark, the treasure items

  public:
	Dungeon();
	~Dungeon();
	void Link(const SimLinks& links) { sim = links; } // once, before the first level
	// depth: the decorations' (scatterDecorations); every tier for a level outside the campaign.
	bool Load(const char* filename, int depth = DECOR_DEPTH_ALL);
	void LoadGrid(const LevelGrid& grid, const char* levelName, int depth = DECOR_DEPTH_ALL); // levelName seeds them
	// Level `number` of the campaign (campaign.h).
	bool LoadCampaignLevel(int number);
	[[nodiscard]] int LevelNumber() const { return levelNumber; }
	// No monster can chase or attack the player now (Monster::Threatens, no wall between): the regeneration amulet
	// heals. Out of combat, as the player feels it.
	[[nodiscard]] bool PlayerSafe() const;
	void SetLevelNumber(int number) { levelNumber = number; } // a save game was loaded
	[[nodiscard]] bool Won() const { return won; }
	void ClearWin() { won = false; } // a new game or a scenario level
	// Puts prop `type` (DECOR_NAMES) on the cell, whatever the level's scatter picked (scenario `prop`). False if the
	// cell is outside the level.
	bool PlaceDecor(int col, int row, int type);
	// A treasure container stands in (col, row): a treasure, or one emptied there (it stays, open).
	[[nodiscard]] bool ContainerStands(int col, int row) const {
		return IsInBounds(col, row) && (MapAt(col, row).type == Treasure || containers[MapIndex(col, row)].emptied);
	}
	// Scenario `container`: the treasure in (col, row) looks like `kind` (CONTAINER_NAMES) in STANCES[stance],
	// whatever the level's scatter picked. False if the cell is outside the level or the stance unknown.
	bool PlaceContainer(int col, int row, int kind, int stance);
	// The highest decoration tier (decor.h) among the level's props, decals and surfaces; -1 for a bare level.
	[[nodiscard]] int DecorTierUsed() const;
	void Update();
	void AnimateMonsters(); // once a tick, after Update: every active monster (Monster::Animate)
	void Draw(const HitboxView* hitboxes = nullptr); // hitboxes: the debug view, nullptr when off
	// The water over everything in it (the player too), see-through: after the player, in Draw's frame.
	void DrawWater();
	bool Move(float dirX, float dirY); // false: blocked, the player did not move
	// Standing in half water (not in the air): the walk slows to WADE_SPEED_FACTOR (WADE_SPEED_FACTOR_HAPI with the
	// Favour of Hapi skill, plan skill-blessing-of-hapi).
	[[nodiscard]] bool PlayerWading() const;
	[[nodiscard]] float PlayerWalkFactor() const;
	// The sprint multiplier of a step (PlayerStats::SprintMoveMultiplier) as it counts here: in half water it is 1 (no
	// sprint), with the Favour of Hapi skill the sprint at WADE_SPRINT_FACTOR_HAPI of the land sprint once
	// PlayerWalkFactor is applied too (plan skill-blessing-of-hapi); WADE_SPRINT_FACTOR_BLESSED with the Blessing of
	// Hapi potion, the full land sprint with both (plan water-blessing-potion).
	[[nodiscard]] float PlayerSprintMultiplier(float sprint) const;
	// The player has the Favour of Hapi skill: a faster wade, and jumping out of half water.
	[[nodiscard]] bool PlayerHasHapi() const;
	// The Blessing of Hapi potion runs (PlayerStats::WaterBlessed).
	[[nodiscard]] bool PlayerWaterBlessed() const;
	// Either of the two lets the player sprint and jump in half water.
	[[nodiscard]] bool PlayerWadesFree() const { return PlayerHasHapi() || PlayerWaterBlessed(); }
	// Wading with the skill and the potion: the sprint drains the stamina at WADE_SPRINT_DRAIN_BOTH.
	[[nodiscard]] bool PlayerHalvedWadeDrain() const {
		return PlayerWading() && PlayerHasHapi() && PlayerWaterBlessed();
	}
	// False while wading (and without the Favour of Hapi or the potion): then the player is told why (at most every
	// WATER_JUMP_HINT_MS).
	bool JumpAllowed();
	// No new attack while hanging on a ladder (PlayerOnLadder): both hands are on the rungs. A swing already under way
	// finishes. tell: say why ("Hands on the rungs"), once per press of the key.
	bool AttackAllowed(bool tell);
	// World units to draw something standing at map x on row's floor down into a water basin: WATER_BASIN_DEPTH in
	// half water, growing over WATER_SINK_RAMP from a dry edge, 0 elsewhere.
	[[nodiscard]] float WaterSink(float x, int row) const;
	// World units the player is drawn down into a water basin (WaterSink, less while above the floor: a fall, a jump).
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
	// A ranged weapon shoots: the missile leaves the hand, height above the player's feet in tiles, facing dir
	// (-1 / +1), aimed at a monster up to aimRange tiles ahead.
	void Shoot(MissileKind kind, int damage, const DamageMix& mix, int dir, float height, float aimRange);
	// The interact key: does what InteractVerb says, nothing on None.
	void Interact();
	// What the interact key does in the player's cell, in this order (plan wall-layer, interact-prompt): the chest or a
	// mimic in ambush (Open; Take once the chest's clip has played, nothing while it plays), then the fixture on the
	// back wall (Pull a lever not pulled yet, Read or Copy an inscription), the brazier with a bow (Light), and the
	// gate last, as it may take the player away (Answer a riddle gate, Enter the exit or a teleporter not in the air,
	// Take the ankh). Keys are picked up on touch: None.
	[[nodiscard]] Interaction InteractVerb() const;
	// The glint over an unread inscription in (col, row) (plan interact-prompt), 0..1: 1 not read yet, dimmer once
	// copied without the skill, 0 translated, too worn to read, or no inscription.
	[[nodiscard]] float InscriptionGlint(int col, int row) const;
	// The jump key, facing dir (-1 / +1): a jump starts unless the player is in the air, dead, wading (JumpAllowed) or
	// out of stamina (told why), or the level is won.
	void StartJump(int dir);
	bool SpawnMonster(int i, int j);
	void Teleport(); // on a teleporter: jump to its partner (useGate starts it under a fade)
	[[nodiscard]] bool Travelling() const { return travel.phase != GateTravel::Phase::None; }
	[[nodiscard]] float TravelFade() const; // 0..1: how far the view is faded into the gate colour
	[[nodiscard]] bool TravelToExit() const { return travel.exit; }
	[[nodiscard]] int KeysHeld() const { return keysHeld; }
	[[nodiscard]] int LevelKeys() const { return levelKeys; } // the HUD's key sockets
	[[nodiscard]] int MonsterBarsShown() const; // living monsters that show their health bar
	// The boss in a fight (alive and alerted), for the HUD bar; null if none.
	[[nodiscard]] const Monster* Boss() const;
	[[nodiscard]] int BossHealth() const; // of the boss in play (alerted or not), 0 if none
	[[nodiscard]] int LivingMinions() const;
	// The living monster nearest the player; null if none.
	[[nodiscard]] const Monster* NearestMonster() const { return nearestMonster(); }
	// Every slot (MAX_MONSTERS), active or not (Monster::Active).
	[[nodiscard]] const Monster* Monsters() const { return monsters; }
	[[nodiscard]] int NearestMonsterHealth() const; // of the living monster nearest the player, 0 if none
	[[nodiscard]] int NearestMonsterPoison() const; // its running poison tiers (Poison::Mask), 0 if none
	[[nodiscard]] bool NearestMonsterBurns() const; // it is on fire (plan fire-damage)
	[[nodiscard]] int ChestCount() const; // treasure chests not opened yet
	[[nodiscard]] int CoffinCount() const; // coffins standing on the level (decorations)
	void HurtBoss(int dmg); // scenario tests: the boss in play takes a hit, as from the player
	// Scenario tests: the living monster nearest the player, or the boss in play, is poisoned, as by the player (it
	// may shrug it off).
	void PoisonNearestMonster(PoisonTier tier);
	void PoisonBoss(PoisonTier tier);
	// A minion's kill: 1 XP while its boss lives (no farming), half its type's xp after the boss died.
	[[nodiscard]] int MinionXP(int xp) const { return bossFight.slot >= 0 ? 1 : xp / 2; }
	// Draft map: the cells within EXPLORE_RADIUS of every tile the player stood on.
	static constexpr int EXPLORE_RADIUS = 1; // cells to each side: a 3 x 3 square
	[[nodiscard]] bool Explored(int col, int row) const { return IsInBounds(col, row) && explored[MapIndex(col, row)]; }
	[[nodiscard]] Tile Cell(int col, int row) const { return MapAt(col, row); }
	void Dump(std::ofstream& f);
	bool LoadDump(std::ifstream& f);
	// Props, decals, torches, ladders and surfaces, seeded by the level's file name, from the decoration tiers its
	// depth unlocks (decor.h: the campaign level, DECOR_DEPTH_ALL outside the campaign).
	void scatterDecorations(const char* levelName, int depth);
	[[nodiscard]] bool bossCoffin(int i, int j) const; // a coffin for the boss's minions stands there
};

#endif
