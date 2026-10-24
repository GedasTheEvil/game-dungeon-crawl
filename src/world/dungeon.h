#ifndef DUNGEON_H
#define DUNGEON_H
#include "../entities/monster.h"
#include <fstream>
#include "../core/timer.h"
#include "../core/gameplay_config.h"
#include "decor_scatter.h"
#include "dungeon_darts.h"
#include "interaction.h"
#include "journal.h"
#include "level.h"
#include "climb_path.h"
#include "curses.h"
#include "view_window.h"
#include "sim_links.h"
#include "sun_beam.h"
#include "zone_look.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct RoyalName;

// Coordinate spaces used throughout the world system:
//   Map space:    float [0, MAP_W] x [0, MAP_H], tile units, used for collision
//   World space:  map * TILE_SIZE, OpenGL units, used for rendering
//   Screen space: projection of world space, origin top-left

// Live monster slots; the campaign's busiest level has 15. No design limit: raise it at will.
constexpr int MAX_MONSTERS = 32;

// A cut-scene's camera this tick (plan cinematic-effects): how far it is from the player in tiles (x right, y up), and
// how far the cut-scene look (letterbox bars, tint, vignette) is in, 0..1.
struct CutShot {
	float dx = 0.f;
	float dy = 0.f;
	float look = 0.f;
};

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
	LevelMeta meta; // the level's, from its file or the save game: its zone (Look)
	bool campaignLevel = false; // the map is campaign level levelNumber (its file or a save of it)
	bool explored[MAP_CELL_COUNT] = {}; // cells on the draft map (map_view.h)
	SimLinks sim; // the player, journal, assets, ... the world was given (Link)
	float mapX, mapY;
	int levelNumber = 1; // the campaign level loaded (campaign.h); a level loaded by path keeps the last number
	bool won = false; // the ankh was taken
	// Solid rock for the drawing: a wall, or outside the level.
	[[nodiscard]] bool isRock(int col, int row) const { return !IsInBounds(col, row) || isWall(MapAt(col, row)); }
	// Round the camera: the player, or where a cut-scene's camera is (cutShot).
	[[nodiscard]] ViewWindow view() const {
		return {static_cast<int>(mapX + cutShot.dx) - 3, static_cast<int>(mapY + cutShot.dy) - 3};
	}
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
	// Trapped chests (plan trapped-chests, dungeon_chest_traps.cpp): openContainer on a TrappedTreasure rolls its
	// contents and its trap (loot.h), the trap goes off unless the trap ward stops it.
	void springChestTrap(int cell);
	std::string fireChestTrap(int cell); // the trap's status line
	void chestSwarm(int cell); // CHEST_SWARM_SIZE scarabs dig out round it
	// A curse from an object (a chest's execration text, a smashed ushabti): laid, its field note; the status line.
	std::string tombCurse(Curse c, const char* execration);
	// A puff of poison gas over a chest (ChestTrap::Gas), for CHEST_GAS_MS: not saved, as the darts in flight.
	struct ChestGas {
		int cell; // map index
		int startMs; // GameClock
		int nextPoisonMs; // the next weak poison for a player in it
	};
	std::vector<ChestGas> chestGas;
	void updateChestGas();
	void drawChestGas(); // after the opaque scene, drawSummonEffects' frame
	// 0 .. 1: how much a closed trapped container in cell shows its tell (darker), by the player's distance.
	[[nodiscard]] float chestTell(int cell) const;
	[[nodiscard]] bool containerShows(int cell) const; // opened and its clip played: the item shows
	// 0 closed .. 1 open, the cell's clip.
	[[nodiscard]] float containerOpening(int cell) const;
	[[nodiscard]] TreasureLook treasureLook(int cell) const;
	bool PullLever(); // interact on a lever cell; false if there is none
	bool readInscription(); // interact on an inscription: into the journal; false if there is none
	// The journal page of the inscription fixture f, translated if the player has the hieroglyph skill; a name
	// cartouche's holds the level's name (plan secret-name-gates). None: too worn to read (no text, no name).
	[[nodiscard]] std::optional<JournalInscription> inscriptionPage(const Fixture& f) const;
	// The name gate opened last (map index) and when (GameClock): its plasma shimmers from the pulse to steady.
	int namedCell = -1;
	int namedAtMs = 0;
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
	// A burst of sand round mon (RoarRules): a charger's storm roar as its stun ends (plan hippo-boss), a sand
	// burrower's spray as it comes up (plan worm-boss). The player on its row near it is hit, shoved away
	// (ShovePlayer) and blinded by sand (SandHaze).
	void sandBurst(const Monster& mon, const RoarRules& rules);
	// The player sliding away from a shove, at SHOVE_SPEED: tiles still to go, the way (-1 / +1). Walls stop it.
	struct Shove {
		float left = 0.f;
		int dir = 0;
	} shove;
	void updateShove();
	// Sand over the screen (SandHaze): from hazeStartMs (GameClock) for hazeMs.
	int hazeStartMs = 0;
	int hazeMs = 0;
	// Sand and quicksand (dungeon_sand.cpp, plan desert-chapter): how long the player has stood sinking in quicksand
	// (0 out of it or in the air), the next quicksand hurt, the end of a sand fall's slow (GameClock).
	int sunkMs = 0;
	int nextSinkHurtMs = 0;
	int sandSlowUntilMs = 0;
	void updateQuicksand();
	// A sand fall landed on cell: the hit and the slow for the player under it, the drift it leaves (the cell turns to
	// sand).
	void landSandFall(int cell);
	// Walkers (dungeon_walkers.cpp). A walker can't step into (col, row): a wall, no floor under it (a pit or a drop),
	// or a trap (spikes, a death trap, a rock fall not yet fallen) unless it is reckless (Courage).
	[[nodiscard]] bool walkerBlocked(int col, int row, bool reckless = false) const;
	// walkerBlocked for this walker, and a light shunner (MonsterKind::shunsLight) can't step into a lit cell either
	// but its own.
	[[nodiscard]] bool walkerStops(const Monster& mon, int col) const;
	// A light shunner keeps out of (col, its row): Lit, but a douser (MonsterKind::douseCells) heeds only the braziers
	// and the sun; the torches and lamps it puts out before it reaches their light.
	[[nodiscard]] bool shunned(const Monster& mon, int col) const;
	// A light shunner standing in the light: the way out along its row (-1 / +1, the nearer dark cell); 0: none.
	[[nodiscard]] int wayOutOfLight(const Monster& mon) const;
	// A lit cell between the walker and the player, or round the player (not its own cell).
	[[nodiscard]] bool litBetween(const Monster& mon) const;
	// The cell a walk-jumper lands on when (col, row) blocks it walking in direction dir: the first one past a gap of
	// up to MONSTER_JUMP_MAX_GAP pits and traps it can walk on. -1: no such cell (a wall, or the gap is too wide).
	[[nodiscard]] int leapLanding(int col, int row, int dir) const;
	[[nodiscard]] float leapTarget(const Monster& mon, int land, int dir) const; // map x of the landing, see Jump
	// A walker on the player's row can get to them that way (dir -1 / +1): walking, a walk-jumper leaping gaps.
	[[nodiscard]] bool walkerReaches(const Monster& mon, int dir) const;
	// A coward that cannot reach the player (Monster::flees): runs away along its row, out of the bow's range.
	void fleeFrom(Monster& mon, int dir);
	// A kiter (KiteRules) on the player's row, dir the way to them: the way it backs off, -1 / +1; 0: it holds (not
	// threatened, back in its band, or cornered).
	[[nodiscard]] int kiteAway(const Monster& mon, int dir) const;
	// The player is nearer than its band and coming at it (or it is backing off and they keep coming).
	[[nodiscard]] bool kiteThreatened(const Monster& mon, int dir) const;
	// A leaping kiter (Babi, plan babi-boss) with a gap behind it (no room to back off), threatened (or, threatened
	// false, nearer than its band, landing in it): the map x it leaps back to over the gap. nullopt: it backs off,
	// holds or fights as a kiter does.
	[[nodiscard]] std::optional<float> leapAwayTarget(const Monster& mon, int dir, bool threatened = true) const;
	bool leapAway(Monster& mon, int dir, bool threatened = true); // leaps to leapAwayTarget; false: none
	// The free run behind a kiter, away from the player (dir), up to upTo tiles: to a wall, a drop, a trap or another
	// monster.
	[[nodiscard]] float kiteRoom(const Monster& mon, int dir, float upTo) const;
	// A kiter that reaches the player that way: backs off, or stands in its band. False: the walker's rules go on (it
	// closes in beyond its band, cornered it closes in nearer than near, or bites the player in reach).
	bool kite(Monster& mon, int dir);
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
	// nor when its slot is reused. Not a minion's (it has no tile), not a mimic's (its tile turns into its chest; the
	// cursed mimic's chest is where it died).
	void markSlain(const Monster& mon);
	void DrawMonsters(const CellRect& drawn); // at their actual position, not their spawn tile
	void drawDecoys(const CellRect& drawn); // with the frame origin of DrawMonsters
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
	int sprintPuffMs = -1000000; // last dust puff of a sprint step (plan sprint-toon-blur-and-extras)
	bool motionEffectsOn = true; // Options > Display > Motion effects (SetMotionEffects): the sprint's dust
	int wadeSoundMs = -1000000; // last splashing step, or step in sand
	void resetMechanisms();
	void updateMechanisms();
	void startOpeningGate(int cell, int startMs);
	// Opens the closed gates of the colour; a cut-scene shows the ones off the screen (startCutScene).
	void openGates(int colour);
	void finishOpenedGates(); // the gates whose motion has run are open
	void bumpGate(int col, int row); // the player walked into a closed gate
	void startRockFall(int cell); // an armed rock fall cell was stepped into: the rumble starts
	void updateRocks();
	void drawKeyTile(int i, int j);
	void drawPageTile(int i, int j);
	void drawGateTile(int i, int j);
	void drawLeverTile(int i, int j);
	// At the cell origin: the columns face the camera, plasma between them; a name gate's pulses (nameGateAlpha).
	void drawTeleporterTile(int i, int j);
	// The plasma's opacity in a teleporter's cell: 1, but for a sealed name gate's breath and the shimmer of one just
	// opened.
	[[nodiscard]] float nameGateAlpha(int cell) const;
	void drawTileContent(int i, int j); // the fixture on the back wall, then what stands in the cell, by tile type
	void drawFixture(int i, int j);
	void drawAnkhTile();
	void drawDoorTile(int i, int j); // a sphinx gate: entrance, exit, riddle or empty
	// The scrolling plasma of the gates, over the quad v (its corners take the texture's (0, 0), (0, 1), (1, 1), (1,
	// 0)).
	void drawPortal(const float normal[3], const float v[4][3], bool red = false, float alpha = 1.f) const;
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
	// Rhythm traps (dungeon_spikes.cpp, plans rhythm-traps, fire-jet-trap): each wall spikes' and fire vent's live
	// state from the clock; coming out, the spikes stab and shove out whoever stands in the cell. A fire vent's jet
	// burns whoever stands in its cell, a jump too (updateTraps).
	void updateRhythmTraps();
	void stabWallSpikes(int col, int row);
	[[nodiscard]] std::optional<float> outOfSpikes(int col, int row, float centre, float half) const;
	void drawWallSpikesTile(int i, int j); // the slotted panel and the spikes, as far out as the cycle has them
	// The fire vent's jet is on in a cell of row the body from left to right overlaps.
	[[nodiscard]] bool inFireJet(float left, float right, int row) const;
	bool playerScorched = false; // the fire jet's line was told this entry (reset on leaving it)
	int jetCarry = 0; // hundredths of a HP of the jet's damage not dealt yet
	void drawFireVentTile(); // the vent low in the back wall
	void drawFireJets(const CellRect& drawn); // the jets and the telegraph's glow, after the opaque tiles
	void addFireVentLights(const CellRect& drawn); // the jets light the corridor, the telegraph glows
	// Sunlight mirrors (dungeon_sun.cpp, plan sunlight-mirrors): the beams as the mirrors and gates stand (traceBeams),
	// the cells they light (map index); a turned mirror swings from its old angle, a sun disc just lit flares.
	std::vector<BeamStep> sunBeams;
	std::vector<char> sunlit = std::vector<char>(MAP_CELL_COUNT, 0);
	struct MirrorSwing {
		int cell;
		int startMs;
		float fromDeg;
	};
	std::vector<MirrorSwing> mirrorSwings;
	std::vector<Motion> litDiscs;
	void traceSunBeams(); // sunBeams and sunlit as the map stands
	// Traces the beams, lights the sun discs they fall on (their gates open), sets the sun burners in them on fire.
	void updateSunBeams();
	bool TurnMirror(); // interact on a mirror: a quarter turn clockwise; false if there is none
	// Degrees round the depth axis, counter-clockwise as the camera sees it, from up: the mirror's face as it reflects
	// the beam that falls on it (from above when none does), swinging after a turn.
	[[nodiscard]] float mirrorAngle(int cell) const;
	[[nodiscard]] float mirrorFaceDeg(int cell) const; // mirrorAngle without the swing
	void drawMirrorTile(int i, int j); // the stand, the mirror and the pointer showing its way
	void drawSunDiscTile(int i, int j);
	void drawSunBeams(); // after the opaque scene, drawSummonEffects' frame
	void addSunLights(const CellRect& drawn);
	// The Mother of Webs (dungeon_webs.cpp, plan camel-spider-boss): awake, a douser (MonsterKind::douseCells) puts
	// out the torches and lamps near it on its row (DecorLayout::doused), a spinner (MonsterKind::webs) spins webs.
	void updateSpinner(const Monster& mon);
	// A web over the floor of cell (map index), spun at laidMs; cut or burnt at goneMs (< 0: whole), gone WEB_CUT_MS
	// or WEB_BURN_MS later. Not saved: a load starts the boss fight over.
	struct Web {
		int cell;
		int laidMs;
		int goneMs = -1;
		bool burnt = false;
	};
	std::vector<Web> webs;
	int nextWebMs = 0; // GameClock: the spinner's next web (one boss per level)
	bool playerWebbed = false; // the player stood in a web on the last tick: the status line was told
	std::vector<Motion> snuffs; // the torches and lamps just put out, their flames dying down (SNUFF_MS)
	void updateWebs(); // the player caught in one, the fire vents' jets burn them, the cut and burnt ones go
	[[nodiscard]] int webAt(int cell) const; // the whole web over the floor of cell (map index): its index, -1 none
	void burnWeb(int cell); // fire on the floor of cell (map index): its web burns
	// A blade's swing over the floor of row from fromX to toX (map x): the webs there are cut. How many.
	int cutWebs(float fromX, float toX, int row);
	[[nodiscard]] bool webFloor(int col, int row) const; // a web can lie there: dry floor, nothing in the cell
	void drawWebs(); // the webs, the burning ones in flames; drawSummonEffects' frame
	Monster monsters[MAX_MONSTERS];
	[[nodiscard]] const Monster* nearestMonster() const; // the living one nearest the player; null if none
	[[nodiscard]] Monster* freeMonsterSlot(); // an empty slot, else the slot of a dead monster; null if none
	// The level's boss fight (at most one boss per level). Monsters are not saved: a load starts the fight over.
	struct BossFight {
		int slot = -1; // monsters[] index of the boss, -1: none in play
		int summoned = 0; // minions summoned this fight, after the first ones
		int nextSummonMs = 0; // GameClock time of the next summon
		int firstLeft = 0; // hatchers: first minions still to come (the egg clusters spawn in view after the boss)
		int hatched = 0; // minions hatched or out of a wall so far: picks the kind (BossRules::smallMinion), the nest
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
	// A sand burrower's spray of sand as it came up (RoarRules spray, plan worm-boss), for Grit::BURST_MS: drawn with
	// the holes.
	std::vector<Hole> sprays;
	// Dust kicked up by a step in sand (plan desert-chapter), for SAND_PUFF_MS: drawn with the holes.
	std::vector<Hole> puffs;
	std::vector<Hole> pours; // sand falls pouring down, from the end of the warning (startMs)
	// Sand on the floor (dungeon_render_sand.cpp, plan desert-chapter).
	[[nodiscard]] float driftHeight(int i, int j, float u, float v) const; // world units over the floor
	void drawSandDrift(int i, int j); // in drawCellSurfaces' frame
	void drawSandEffects(); // the steps' dust, a worm's ripple, the sand falls; drawSummonEffects' frame
	void drawRipple(float x, float y, float z, float size, int now) const; // size: 1 a worm's
	// A shaft of daylight falls through the cracked sandstone ceiling of (i, j) (CEILING_SANDSTONE_CRACKED, picked by
	// scatterSurfaces).
	[[nodiscard]] bool lightShaft(int i, int j) const;
	void drawHole(float x, float y, float z, float fade) const;
	// Map x a burrower comes up at: BURROW_BEHIND past the player, away from where it is; else as far in front of
	// them. False: neither is floor it can stand on.
	[[nodiscard]] bool burrowTarget(const Monster& mon, float& outX) const;
	// Map x a sand burrower (MonsterKind::sandBurrow) in sand comes up at: under the player on its row within range
	// tiles, through sand all the way, or past tiles beyond them if that is sand too; for a lurker (edge) at the end of
	// its sand nearest a player off the sand too. False: none.
	[[nodiscard]] bool sandTarget(const Monster& mon, float range, bool edge, float past, float& outX) const;
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
	// A volley's later shots (plan babi-boss) wait at the release point until their spawnMs, then are aimed.
	struct MonsterShot {
		float x, y, vx, vy;
		const MonsterType* from; // its damage, mix and poison
		Rgb colour; // RangedRules::colour
		int lastMs; // GameClock time of the last update
		float flown = 0.f; // tiles
		int splatMs = -1; // GameClock time it hit a wall or the floor; < 0: flying
		int spawnMs = 0; // GameClock time it was shot: the seed of its flames
		bool big = false; // a volley thrower's big coal (VolleyRules): at the feet, it sets the floor burning
		[[nodiscard]] bool waiting(int now) const { return now < spawnMs; }
	};
	std::vector<MonsterShot> shots;
	void shootAtPlayer(const Monster& mon, float x, float y);
	void aimShot(MonsterShot& v) const; // at the player as they stand: the chest, a big coal the feet
	void updateShots();
	// A floor burning where Babi's big coal landed (plan babi-boss, dungeon_blaze.cpp): the cell (map index) from
	// startMs to endMs. The player standing in it, not in a jump, takes BLAZE_DAMAGE every BLAZE_TICK_MS and burns on.
	// Not saved.
	struct Blaze {
		int cell;
		int startMs;
		int endMs;
	};
	std::vector<Blaze> blazes;
	int nextBlazeHitMs = 0; // GameClock: the player's next hit standing in a blaze
	bool playerBlazed = false; // the player stood in a blaze on the last tick: the status line was told
	void blazeAt(float x, float y, int ms); // the floor of the cell under (x, y) burns, unless it is wet or no floor
	void updateBlazes();
	[[nodiscard]] bool blazeFloor(int col, int row) const; // a dry floor cell
	void drawBlazes(); // drawSummonEffects' frame
	void addBlazeLights();
	void addVenomLights();
	void addFireLights(); // the fire arrows' and the burning monsters' (plan fire-damage)
	void drawBurning(); // flames on the burning monsters and the fire arrows; drawSummonEffects' frame
	void drawShots(); // with the frame origin of DrawMonsters
	[[nodiscard]] bool clearRow(float fromX, float toX, int row) const; // no solid cell between the two on row
	// A dung ball (plan scarab-dung-ball, dungeon_balls.cpp): kicked off by a roller (Monster::UpdateRoll), it rolls
	// along the floor of its row; map units, (x, y) the bottom of the ball.
	struct DungBall {
		float x, y;
		int dir; // -1 / +1
		RollRules rules; // its roller's
		int from; // the roller's MonsterTypeId: the journal's
		int startMs; // GameClock time of the kick: the seed of its flames and crumbs
		float rolled = 0.f; // tiles: the spin, the end of its roll
		float vy = 0.f; // tiles per second: falling into a pit (< 0)
		bool falling = false;
		int breakMs = -1; // GameClock time it broke; < 0: rolling
		bool sank = false; // broke in water: no crumbs
	};
	std::vector<DungBall> balls;
	bool updateRoller(Monster& mon); // the windup and the kick; true: busy with it, no walk or bite
	void updateBalls();
	void breakBall(DungBall& ball, bool inWater);
	void drawBalls(); // the balls rolling and in the windups; with the frame origin of DrawMonsters
	void drawBallEffects(); // Khepri's flames, the crumbs of the broken ones; drawSummonEffects' frame
	void addBallLights(); // the burning balls'; same frame as addLights

	void dropChest(const Monster& mon, ItemKind weapon); // RollKillDrop
	// Spike and death trap tiles hurt the player and the monsters standing in them (TrapHurt).
	TrapHurt trapHurt; // the player's
	// (x, y) in map units is in a spike or death trap's hitbox: TRAP_HITBOX_X/Y_SCALE x the trap's scale round the
	// tile's bottom centre.
	// The trap it stands in: its tile type (Spike or Death) and x's offset from its middle in half widths (-1 to 1).
	struct TrapContact {
		int type;
		float offset;
		float centre; // its middle's map x
		float half; // its hitbox's half width
	};
	[[nodiscard]] std::optional<TrapContact> inTrap(float x, float y) const;
	float trapLastX = 0.f; // the player's x on the tick before (updateTraps): the way covered in a trap
	void updateTrapCrossing(const std::optional<TrapContact>& here);
	int trapBellCarry = 0; // hundredths of a HP of the crossing's damage not dealt yet
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
	// Cut-scenes (dungeon_cutscene.cpp, plan cinematic-effects): the camera goes to the gates a lever or a boss's death
	// opened off the screen, holds on each while it opens, and comes back. The world stands still meanwhile, but for
	// the gates.
	struct CutScene {
		std::vector<int> stops; // gate cells (map index), in the order shown; empty: none playing
		int startMs = 0;
	} cutScene;
	bool cutScenesOn = false; // Options > Display > Cut-scenes (SetCutScenes)
	CutShot cutShot; // this tick's camera, set by updateCutScene
	// The cut-scene for the gates just opened (cells): the nearest CUTSCENE_MAX_STOPS off the screen. Each starts to
	// open as the camera comes to it.
	void startCutScene(const std::vector<int>& opened);
	void updateCutScene();
	[[nodiscard]] int cutSceneLengthMs() const;
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
	[[nodiscard]] const std::string& LevelTitle() const { return meta.title; } // empty: none
	// The look of the level's zone (plan zone-looks, LevelMeta): light, fog, ambience. DEFAULT_LOOK in none.
	[[nodiscard]] const ZoneLook& Look() const { return levelLook(meta.zone); }
	[[nodiscard]] bool CampaignLevel() const { return campaignLevel; } // not a test or generated level
	// No monster can chase or attack the player now (Monster::Threatens, no wall between): the regeneration amulet
	// heals. Out of combat, as the player feels it.
	[[nodiscard]] bool PlayerSafe() const;
	// Not the moment for an autosave (plan autosave): the player is poisoned, burning, held (PlayerHeldBy), on a
	// ladder, in the air (a jump, a fall), travelling through a gate or in quicksand.
	[[nodiscard]] bool PlayerInDanger() const;
	void SetLevelNumber(int number) { levelNumber = number; } // a save game was loaded
	// Puts the player in cell (col, row), on its floor (scenario `level ... at`, play scripts that start at a feature).
	// False: not a cell to stand in (a wall, no floor and no ladder).
	bool PlacePlayer(int col, int row);
	[[nodiscard]] bool Won() const { return won; }
	// The dung balls rolling, falling or broken (plan scarab-dung-ball), for the tests.
	[[nodiscard]] const std::vector<DungBall>& Balls() const { return balls; }
	void ClearWin() { won = false; } // a new game or a scenario level
	// Puts prop `type` (DECOR_NAMES) on the cell, whatever the level's scatter picked (scenario `prop`). False if the
	// cell is outside the level.
	bool PlaceDecor(int col, int row, int type);
	// The prop standing in the cell (DECOR_NAMES), -1 for none.
	[[nodiscard]] int PropAt(int col, int row) const {
		return IsInBounds(col, row) ? decoration.decor[MapIndex(col, row)].type : -1;
	}
	// A treasure container stands in (col, row): a treasure, or one emptied there (it stays, open).
	[[nodiscard]] bool ContainerStands(int col, int row) const {
		return IsInBounds(col, row) && (isTreasure(MapAt(col, row)) || containers[MapIndex(col, row)].emptied);
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
	bool Move(float dirX, float dirY); // false: blocked or held (PlayerHeldBy), the player did not move
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
	// Standing in sand or quicksand (not in the air): the walk slows to SAND_SPEED_FACTOR, no sprint (plan
	// desert-chapter).
	[[nodiscard]] bool PlayerInSand() const;
	[[nodiscard]] bool PlayerInQuicksand() const { return PlayerInSand() && inQuicksand(Map(mapX, mapY)); }
	// How long the player has been sinking in quicksand, ms; 0 out of it.
	[[nodiscard]] int PlayerSunkMs() const { return sunkMs; }
	// A walk step may sprint: not wading (without the Favour or the Blessing of Hapi), not in sand.
	[[nodiscard]] bool PlayerCanSprint() const { return (!PlayerWading() || PlayerWadesFree()) && !PlayerInSand(); }
	// Wading with the skill and the potion: the sprint drains the stamina at WADE_SPRINT_DRAIN_BOTH.
	[[nodiscard]] bool PlayerHalvedWadeDrain() const {
		return PlayerWading() && PlayerHasHapi() && PlayerWaterBlessed();
	}
	// False while wading (and without the Favour of Hapi or the potion): then the player is told why (at most every
	// WATER_JUMP_HINT_MS).
	bool JumpAllowed();
	// No new attack while hanging on a ladder (PlayerOnLadder): both hands are on the rungs, nor in a crocodile's jaws
	// (PlayerHeldBy). A swing already under way finishes. tell: say why ("Hands on the rungs"), once per press of the
	// key.
	bool AttackAllowed(bool tell);
	// The monster that holds the player (Monster::Holding, plans crocodile-hold-bite, giant-scorpion-grab); null if
	// none. Held, the player cannot walk, jump, sprint, drink or open a screen; attack only a scorpion's claws.
	[[nodiscard]] const Monster* PlayerHeldBy() const;
	// Degrees the held view rolls: with the death roll's ticks (HELD_ROLL_DEG), a shake in the claws (HELD_SHAKE_DEG);
	// 0 when not held.
	[[nodiscard]] float HeldViewRoll() const;
	// World units to draw something standing at map x on row's floor down into a water basin: WATER_BASIN_DEPTH in
	// half water, growing over WATER_SINK_RAMP from a dry edge, 0 elsewhere.
	[[nodiscard]] float WaterSink(float x, int row) const;
	// World units the player is drawn down into a water basin (WaterSink, less while above the floor: a fall, a jump),
	// or into quicksand (QuicksandSink), or the sand of a swallow (SwallowSink).
	[[nodiscard]] float PlayerSink() const;
	// World units the player has sunk into quicksand: down to QUICKSAND_SINK_DEPTH tiles as they get stuck.
	[[nodiscard]] float QuicksandSink() const;
	// World units the player is pulled down into the sand, held by a swallower (HoldRules::swallow, plan worm-boss).
	[[nodiscard]] float SwallowSink() const;
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
	// The level's royal name in this run (plan secret-name-gates, names.h): the name gate asks for it, the name
	// cartouche shows it. Null: no names loaded.
	[[nodiscard]] const RoyalName* LevelName() const;
	// The right name was spoken at the name gate in the player's cell: it opens for good. False: no name gate there.
	bool SpeakName();
	// The jump key, facing dir (-1 / +1): a jump starts unless the player is in the air, dead, held (PlayerHeldBy),
	// wading (JumpAllowed) or out of stamina (told why), or the level is won.
	void StartJump(int dir);
	bool SpawnMonster(int i, int j);
	void Teleport(); // on a teleporter: jump to its partner (useGate starts it under a fade)
	[[nodiscard]] bool Travelling() const { return travel.phase != GateTravel::Phase::None; }
	[[nodiscard]] float TravelFade() const; // 0..1: how far the view is faded into the gate colour
	[[nodiscard]] bool TravelToExit() const { return travel.exit; }
	// Pushes the player tiles along the row, dir -1 / +1, sliding (a storm roar). Not while held.
	void ShovePlayer(int dir, float tiles);
	// 0..1: how thick the sand over the screen is (a storm roar), fading out.
	[[nodiscard]] float SandHaze() const;
	[[nodiscard]] int DustPuffs() const { return static_cast<int>(puffs.size()); } // steps' dust still in the air
	void SetMotionEffects(bool on) { motionEffectsOn = on; }
	void SetCutScenes(bool on) { cutScenesOn = on; } // off: remote gates open unseen
	[[nodiscard]] bool CutScenePlaying() const { return !cutScene.stops.empty(); }
	[[nodiscard]] CutShot CutSceneShot() const { return cutShot; }
	void SkipCutScene(); // back to the player at once; the gates go on opening
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
	// A wall torch on (col, row) or none, over whatever the scatter picked (the scenario command torch). False: out of
	// the level.
	bool PlaceTorch(int col, int row, bool on);
	// Within LIGHT_SHUN_CELLS along the row of a flame among the decorations (a wall torch, a brazier, an oil lamp) or
	// of a sunlit cell (Sunlit): a light shunner keeps out (plan camel-spider). torches false: only a brazier's flame
	// and the sun count.
	[[nodiscard]] bool Lit(int col, int row, bool torches = true) const;
	// The torch or lamp in (col, row) was put out (DecorLayout::doused, plan camel-spider-boss).
	[[nodiscard]] bool Doused(int col, int row) const {
		return IsInBounds(col, row) && decoration.doused[MapIndex(col, row)];
	}
	// A whole web lies over the floor of (col, row) (plan camel-spider-boss).
	[[nodiscard]] bool WebAt(int col, int row) const { return IsInBounds(col, row) && webAt(MapIndex(col, row)) >= 0; }
	// Scenario tests: a web over the floor of (col, row). False: no floor for one there (webFloor), or one lies there.
	bool PlaceWeb(int col, int row);
	// The player stands in a web (not in the air): the walk slows to WEB_SLOW_FACTOR (PlayerWalkFactor).
	[[nodiscard]] bool PlayerInWeb() const;
	// The floor of (col, row) burns (Babi's big coal, plan babi-boss).
	[[nodiscard]] bool BlazeAt(int col, int row) const;
	// Scenario tests: the floor of (col, row) burns for ms. False: no dry floor there (blazeFloor).
	bool PlaceBlaze(int col, int row, int ms);
	[[nodiscard]] bool PlayerInBlaze() const; // the player stands in a burning floor cell, not in the air
	// Monster shots in flight or waiting in a volley (plan babi-boss); big: only the big coals.
	[[nodiscard]] int ShotCount(bool big = false) const;
	// A beam of sunlight runs through (col, row) now (plan sunlight-mirrors); it counts as light for Lit too.
	[[nodiscard]] bool Sunlit(int col, int row) const {
		return IsInBounds(col, row) && sunlit[static_cast<size_t>(MapIndex(col, row))] != 0;
	}
	// Scenario `mirror`: the mirror in (col, row) points dir (BeamDir), at once. False: no mirror there.
	bool SetMirror(int col, int row, int dir);
	void HurtBoss(int dmg); // scenario tests: the boss in play takes a hit, as from the player
	// Scenario tests: the living monster nearest the player, or the boss in play, is poisoned, as by the player (it
	// may shrug it off).
	void PoisonNearestMonster(PoisonTier tier);
	// Scenario `grab`: the living monster nearest the player starts a grab's tell now (Monster::StartGrab). False: it
	// does not grab, or is grabbing already.
	bool GrabNearest();
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
