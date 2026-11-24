#ifndef DUNGEON_H
#define DUNGEON_H
#include "../monster/monster.h"
#include <fstream>
#include "../../core/timer.h"
#include "../../level/tuning/gameplay_shared.h"
#include "../../level/decor_scatter.h"
#include "dungeon_darts.h"
#include "../../level/rules/interaction.h"
#include "../../level/rules/items.h"
#include "../../level/rules/journal.h"
#include "../../level/level.h"
#include "../../level/rules/run_stats.h"
#include "../../level/climb_path.h"
#include "../../level/rules/curses.h"
#include "../../level/shrines.h"
#include "../../level/view_window.h"
#include "sim_links.h"
#include "cut_scene.h"
#include "gate_travel.h"
#include "stargate_dial.h"
#include "mechanisms.h"
#include "projectiles.h"
#include "hazards.h"
#include "../../level/sun_beam.h"
#include "../../level/zone_look.h"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct RoyalName;

// Coordinate spaces used throughout the world system:
//   Map space:    float [0, MAP_W] x [0, MAP_H], tile units, used for collision
//   World space:  map * TILE_SIZE, OpenGL units, used for rendering
//   Screen space: projection of world space, origin top-left

// The sealed rooms of a level (plan heka-surplus): the open cells joined to a teleporter's by no other way, with no
// entrance or exit in them, at most a few dozen cells (the hidden rooms and the name gates' secret rooms). cells: a
// whole level.
[[nodiscard]] std::vector<std::vector<int>> secretRooms(const Tile* cells);

// Live monster slots; the campaign's busiest level has 15. No design limit: raise it at will.
constexpr int MAX_MONSTERS = 32;

// What the hitbox debug view (F3, scenario 'hitboxes on') shows of the weapon: its reach in tiles, measured from the
// player's box edge (melee) or the centre (the bow).
struct HitboxView {
	float reach = 0.f;
	bool fromEdge = true;
	int facing = 1; // the player looks -1 left, +1 right
};

// What struck a monster, for the archer skills (plan archer-skills): a blade, a bow's arrow, another missile (sling,
// stick, javelin).
enum class HitBy : std::uint8_t { Melee, Bow, Other };

class Dungeon {
	friend class DungeonView; // its drawing (src/graphics/dungeon/dungeon_view.h, plan dungeon-view-split)
	friend class DungeonCheats; // what only the tests do (dungeon_cheats.h)

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
	bool peace = false; // scenario `peace`: the monsters leave the player alone (SetPeace)
	// The monsters stand down: no attack, no step, no shot, no summon (after the win, or in peace).
	[[nodiscard]] bool monstersStandDown() const { return won || peace; }
	// Solid rock for the drawing: a wall, or outside the level.
	[[nodiscard]] bool isRock(int col, int row) const { return !IsInBounds(col, row) || isWall(MapAt(col, row)); }
	// Round the camera: the player, or where a cut-scene's camera is (cutScene.Shot()).
	[[nodiscard]] ViewWindow view() const {
		const CutShot shot = cutScene.Shot();
		return {static_cast<int>(mapX + shot.dx) - 3, static_cast<int>(mapY + shot.dy) - 3};
	}
	bool IsInBounds(int col, int row) const;
	int MapIndex(int col, int row) const;
	Tile MapAt(int col, int row) const;
	void SetMapBAtPlayer(int value);
	void resetPlayerMotion(); // the player was placed on a level: no jump or fall carries over
	// Up or down pressed in a jump on its way down through a ladder cell (plan jump-onto-ladder): the hands catch the
	// rungs, the jump ends at that height. False: no catch, the press does not climb in the air.
	bool catchRungs();
	void exploreAroundPlayer();
	void UpdateMovementState();
	// Once a tick (plan crouch): the down key held crouches the player on the floor (not in a ladder's cell, where it
	// climbs down, nor in a jump or a fall); let go, they stand up once there is room over the crouched body
	// (roomToStand).
	void updateCrouch();
	void UpdateMonsters();
	bool PickUp(); // not the car... just take an item away; false if there is none
	// Treasure containers (plan treasure-containers, containers.h): what was done to the one in each cell.
	struct ContainerCell {
		bool opened = false; // its clip started at openedMs: the item shows once it has played
		bool emptied = false; // the item was taken: the open container stays
		bool dropped = false; // a kill's or a dead mimic's chest: droppedLook(droppedKind), open from the start
		Container droppedKind = Container::Chest; // the chest, or a dead mimic jar's smashed jar
		int openedMs = 0; // GameClock
		bool cursed = false; // a dead cursed mimic's chest: its look (plan cursed-mimic-chest-look)
	};
	ContainerCell containers[MAP_CELL_COUNT];
	static constexpr int CONTAINER_OPENED_LONG_AGO_MS = 1000000; // a loaded open container's clip has played
	static char containerChar(const ContainerCell& c); // in a save (dungeon_io.cpp)
	static ContainerCell containerFromChar(char c);
	void openContainer(); // Interact Open on a closed treasure: its clip starts
	// A chest a kill or a dead mimic leaves on cell (map index): open, its item showing. kind: a mimic jar's jar;
	// cursed: a cursed mimic's chest, drawn in its colours.
	void placeDroppedChest(int cell, ItemFileId item, Container kind = Container::Chest, bool cursed = false);
	// Trapped chests (plan trapped-chests, dungeon_chest_traps.cpp): openContainer on a TrappedTreasure rolls its
	// contents and its trap (loot.h), the trap goes off unless the trap ward stops it.
	void springChestTrap(int cell);
	[[nodiscard]] bool disarmed(); // the Disarm trap skill's roll stops a container's trap or curse
	std::string fireChestTrap(int cell); // the trap's status line; empty for a curse, which tells its own
	void chestSwarm(int cell); // CHEST_SWARM_SIZE scarabs dig out round it
	// A curse from an object (a chest's execration text, a smashed ushabti): the execration's status line, then the
	// curse laid (its own line) and its field note.
	void tombCurse(Curse c, const char* execration);
	void updateChestGas();
	// 0 .. 1: how much a closed trapped container in cell shows its tell (darker), by the player's distance.
	[[nodiscard]] float chestTell(int cell) const;
	// Tomb-curse coffins (plan tomb-curse-containers): a coffin with an execration text on its lid, an inscribed one
	// or a trapped one (whose trap is its curse). Opening an inscribed one curses unless the trap ward stops it; its
	// loot is a trapped chest's.
	[[nodiscard]] bool cursedCoffin(int cell) const;
	void coffinCurse();
	// 0 .. 1: how much of the text shows on a cursed coffin's lid in cell, by the player's distance (as chestTell).
	[[nodiscard]] float coffinTell(int cell) const;
	[[nodiscard]] float tellNear(int cell) const; // 1 within CHEST_TELL_NEAR of the cell, 0 from CHEST_TELL_FAR
	// The Wisdom of Seshat and Disarm trap (plan heka-surplus, dungeon_seshat.cpp): the level's sealed rooms
	// (secretRooms, found on load), put on the draft map with the player near; the red contour's strength, 0 .. 1, by
	// the player's distance (1 up to OUTLINE_FADE tiles before `far`, 0 from it): a closed trapped container's, a
	// disguised mimic's; DungeonView::queueContour draws it.
	std::vector<std::vector<int>> secrets;
	void findSecretRooms();
	void revealSecretRooms();
	[[nodiscard]] float outlineNear(float col, float row, float far) const;
	[[nodiscard]] float containerOutlineAt(int cell) const;
	[[nodiscard]] float mimicOutline(const Monster& mon) const;
	[[nodiscard]] bool containerShows(int cell) const; // opened and its clip played: the item shows
	// 0 closed .. 1 open, the cell's clip.
	[[nodiscard]] float containerOpening(int cell) const;
	[[nodiscard]] TreasureLook treasureLook(int cell) const;
	bool PullLever(); // interact on a lever cell; false if there is none
	bool readInscription(); // interact on an inscription: into the journal; false if there is none
	// The journal page of the inscription fixture f, translated if the player has the hieroglyph skill; a name
	// cartouche's holds the level's name (plan secret-name-gates). None: too worn to read (no text, no name).
	[[nodiscard]] std::optional<JournalInscription> inscriptionPage(const Fixture& f) const;
	// The name cartouche's page: the level's royal name; none without a name.
	[[nodiscard]] std::optional<JournalInscription> nameCartouchePage(bool translated) const;
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
	// UpdateMonsters' phases per monster, in this order (plan refactor-update-monsters). A bool phase returns true when
	// the monster is done for this tick: the later phases and updateWalker skip it.
	void noteMonsterEvents(Monster& mon); // its pack call, its death, its weapon chest
	void updateMonsterTerrain(Monster& mon); // the step out of the water, the terrain, the splash, the rungs
	bool updateMonsterAirborne(Monster& mon); // killed on the rungs (it falls), a flyer, a leap
	bool updateMimicChest(Monster& mon); // a dead mimic leaves its chest (or jar) and is cleared
	bool updateMonsterAttack(Monster& mon); // lurk, stun, the bite, the hold, the locomotions, the shot, the kite
	bool updateMonsterBite(Monster& mon); // the bite's wind-up and the hold; true while either runs
	bool updateMonsterRanged(Monster& mon); // the shot and the kite; true when it shoots or backs off
	// UpdateMonsters' part per locomotion, each in its file (dungeon_walkers.cpp, dungeon_flight.cpp,
	// dungeon_burrow.cpp, dungeon_charge.cpp). updateBurrower, updateCharger: true while it burrows or charges (nothing
	// else moves it).
	void updateWalker(Monster& mon);
	void updateFlyer(Monster& mon);
	// A flyer's band in the shaped cells round it (FlightGoal::banded); true: the opening ahead is lower than it.
	[[nodiscard]] bool flightBand(const Monster& mon, float probe, FlightGoal& goal) const;
	bool updateBurrower(Monster& mon);
	[[nodiscard]] bool solidFloorAt(float x, int row) const; // a solid sub-cell at the bottom of (x, row)
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
	// A dazed player lets go of the ladder (plan giant-bat-screech): they fall through its cells until they stand.
	bool lostGrip = false;
	[[nodiscard]] float groundWalkFactor() const; // PlayerWalkFactor but the daze
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
	// Sandstorm halls (dungeon_gusts.cpp, plan sandstorm-halls). The way a gust blowing in the cell at (x, row) pushes,
	// -1 / +1; 0: calm, or no gust there.
	[[nodiscard]] int gustAt(float x, int row) const;
	// A gust pushes the player (not held, not on a ladder) and the light monsters in it downwind, walls stop them; the
	// storm round the player (storm, muffle) follows stormTarget.
	void updateGusts();
	// The storm round the player now, 0..1: the haze from the gusts near on their row (the nearer and the harder they
	// blow, the more), and how near a blowing one is (blow).
	void stormTarget(float& haze, float& blow) const;
	float storm = 0.f; // StormHaze
	float muffle = 0.f; // SoundMuffle
	bool playerInGust = false; // last tick: a gust's first push says so
	// Walkers (dungeon_walkers.cpp). A walker can't step into (col, row): a wall, no floor under it (a pit or a drop),
	// or a trap (spikes, a death trap, a rock fall not yet fallen) unless it is reckless (Courage).
	[[nodiscard]] bool walkerBlocked(int col, int row, bool reckless = false) const;
	// walkerBlocked for this walker, and a light shunner (MonsterKind::shunsLight) can't step into a lit cell either
	// but its own.
	[[nodiscard]] bool walkerStops(const Monster& mon, int col) const;
	// walkerStops for a cell past the one its front is in: also a shape its body does not pass going dir
	// (shapePasses, plan subcell-monsters).
	[[nodiscard]] bool walkerStopsAhead(const Monster& mon, int col, int dir) const;
	// Sub-cells (plan subcell-monsters). The floor under mon's body, in tiles over its row's floor: the highest solid
	// sub-cell top under any part of it, a little in from its sides (as the player's foot on a step).
	[[nodiscard]] float monsterFloor(const Monster& mon) const;
	// World units from its head, at rest, up to the ceiling over its body (NO_CEILING: none within a leap's reach).
	[[nodiscard]] float leapRoom(const Monster& mon) const;
	// leapRoom's ceiling: the room in world units, the map point found solid (plan ceiling-spikes reads its cell).
	struct LeapCeiling {
		float room, x, y;
	};
	[[nodiscard]] std::optional<LeapCeiling> leapCeiling(const Monster& mon) const;
	[[nodiscard]] float leapHead(const Monster& mon) const; // map y of its head at rest
	// The ceiling trap (CeilingSpikes, CeilingDeath) leapRoom's ceiling is, NoObject for none (plan ceiling-spikes).
	[[nodiscard]] int leapCeilingTrap(const Monster& mon) const;
	// A leap of mon from where it stands to toX (tile-local, as Monster::Jump) would pass under a ceiling trap in a
	// leap's reach over its head: a coward does not take it, a reckless one does (plan ceiling-spikes).
	[[nodiscard]] bool leapUnderSpikes(const Monster& mon, float toX) const;
	// leapUnderSpikes for a monster that shuns it (not reckless).
	[[nodiscard]] bool shunsLeap(const Monster& mon, float toX) const {
		return !mon.reckless() && leapUnderSpikes(mon, toX);
	}
	// What the sub-cell column at map x on its row asks of mon's body stepping into it: nothing, a hop (a rise within
	// its kind's step), or a stop (a higher rise, an opening lower than its height).
	enum class StepAhead : unsigned char { Walk, Hop, Stop };
	[[nodiscard]] StepAhead stepAt(const Monster& mon, float x) const;
	// stepAt the column its front steps into next going dir (its box edge and a walk step on).
	[[nodiscard]] StepAhead stepAhead(const Monster& mon, int dir) const;
	// A leaper at a step it can take going dir: it jumps onto it, its front landing in the middle of the step's column.
	// False: no step there, or it cannot jump now.
	bool hopOnto(Monster& mon, int dir);
	// A light shunner keeps out of (col, its row): Lit, but a douser (MonsterKind::douseCells) heeds only the braziers
	// and the sun; the torches and lamps it puts out before it reaches their light.
	[[nodiscard]] bool shunned(const Monster& mon, int col) const;
	// A light shunner standing in the light: the way out along its row (-1 / +1, the nearer dark cell); 0: none.
	[[nodiscard]] int wayOutOfLight(const Monster& mon) const;
	// A lit cell between the walker and the player, or round the player (not its own cell).
	[[nodiscard]] bool litBetween(const Monster& mon) const;
	// How (col, its row) suits mon for the avoidance (a fire vent counts always).
	[[nodiscard]] TerrainMood moodAt(const Monster& mon, int col) const;
	// A walker that is not chasing the player (idle, fleeing, backing off) does not step into a hostile cell from a
	// better one: it is a wall to it.
	[[nodiscard]] bool terrainShuns(const Monster& mon, int col) const;
	// A walker standing in a hostile cell: the way (-1 / +1) to a better one. Fighting (the player in its reach), the
	// nearest within TERRAIN_STEP_OUT_CELLS from which it still reaches them; idle (the player off its row), the
	// nearest within TERRAIN_WALK_OUT_CELLS. 0: stay. to: map x it stops at.
	[[nodiscard]] int wayOutOfTerrain(const Monster& mon, int dir, float& to) const;
	// The cell a walk-jumper lands on when (col, row) blocks it walking in direction dir: the first one past a gap of
	// up to MONSTER_JUMP_MAX_GAP pits and traps it can walk on. -1: no such cell (a wall, or the gap is too wide).
	// With a body (plan subcell-monsters): a shaped landing cell it passes going dir.
	[[nodiscard]] int leapLanding(int col, int row, int dir, BodyFit body = {}) const;
	[[nodiscard]] float leapTarget(const Monster& mon, int land, int dir) const; // map x of the landing, see Jump
	// A pouncer (PounceRules) on the player's row, dir the way to them, in its band with the way clear: it leaps into
	// bite reach of them. False: not now.
	bool pounce(Monster& mon, int dir);
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
	// A shooter under a step (plan anubis-stuck-under-step): a walker with a ranged attack and no KiteRules, on the
	// player's row in its range, that cannot bite them (lineUpSide). Back: its shot from here hits the
	// terrain first, and a spot behind it (within its range and its free run, kiteRoom) clears the line; or none does
	// and the player stands up on a step: it backs off to its range's edge. Hold: the line is clear here and one step
	// on would lose it, or it is at its range's edge. None: the walker's rules (in open ground, cornered, or behind
	// cover on its own floor).
	enum class LineUp : std::uint8_t { None, Back, Hold };
	[[nodiscard]] LineUp lineUpMove(const Monster& mon, int dir) const;
	// Such a shooter that cannot bite the player: the way to them, -1 / +1; 0: not one, or it bites. dir:
	// Monster::attackDirection; 0 (near by the row) counts when its swing misses the player up on a step.
	[[nodiscard]] int lineUpSide(const Monster& mon, int dir) const;
	bool lineUp(Monster& mon, int dir); // does lineUpMove's; false: None
	// A walker gets to where it bites the player from that way: walkerReaches, and no step it cannot take on the way,
	// in the player's own cell too.
	[[nodiscard]] bool walksToBite(const Monster& mon, int dir) const;
	// The shot of mon, standing with its centre at map x at and turned dir to the player, reaches them: no wall and
	// no solid sub-cell on the line from its release point to their chest (Dungeon::aimShot).
	[[nodiscard]] bool shotLineClear(const Monster& mon, float at, int dir) const;
	// A climber that has seen the player follows its path to them across floors (ClimbMap) while they are off its row
	// or out of its reach along it; dir: Monster::attackDirection. On the rungs with the player in reach, it steps off
	// them (rungsExit). False: the walker's rules go on (no path, given up).
	bool followPath(Monster& mon, int dir);
	bool climbDown(Monster& mon); // a climber with no way on, hanging over the shaft: down to the floor below
	// A climber on the rungs with the player in reach: the way off them to a floor, -1 / +1; 0: none.
	[[nodiscard]] int rungsExit(const Monster& mon) const;
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
	// (mix; nullptr: untyped), and a kill is rewarded (rewardKill). exactHit (a test weapon, plan test-weapons; a typed
	// hit): the hit takes exactly 1 HP, not what the amulets, resistances, a stun or the weapon's damage make of dmg;
	// the fire still sets it burning and the type's side effects still roll.
	// by: what struck, for the archer skills (plan archer-skills): a bow's arrow, a blade, or another missile.
	// `weapon`: the one that dealt it, counted in the run's stats (none: a cheat's or a test's hit).
	void playerHit(
		Monster& mon,
		int dmg,
		const DamageMix* mix,
		bool exactHit = false,
		HitBy by = HitBy::Melee,
		std::optional<ItemKind> weapon = std::nullopt
	);
	void touchPoison(const Monster& mon); // the plant's sap, after a melee hit
	// The venom amulet's roll after a weapon hit, one with Sting of Serket (a bow's) or Fang of Wadjet (a blade's).
	void venomHit(Monster& mon, HitBy by);
	// The Stun skill's roll after a weapon hit that did not kill (plan skill-stun, Monster::StunHit); the first stun
	// says so and writes its field note.
	void stunRoll(Monster& mon, const DamageMix& mix, bool wasLurking);
	// The Bleed skill's roll after a weapon hit that did not kill (plan skill-bleed), the same way; with Arrows of
	// Satet a bow's hit rolls its bleed in the same pass, the higher chance of each tier counts.
	void bleedRoll(Monster& mon, const DamageMix& mix, HitBy by);
	void rewardKill(Monster& mon); // the player killed it: journal, XP, maybe a weapon chest
	// A killed monster's spawn tile turns into its slain marker (slainObject): it does not come back, not after a load
	// nor when its slot is reused. Not a minion's (it has no tile), not a mimic's (its tile turns into its chest; the
	// cursed mimic's chest is where it died).
	void markSlain(const Monster& mon);
	[[nodiscard]] bool inView(const Monster& mon) const;
	void noteSeenMonsters(); // the monsters on screen go in the journal
	[[nodiscard]] bool dryOpen(int col, int row) const; // walkable and not water: a basin's edge
	Tile Map(float x, float y) const;
	// The player's hitbox (Player::HalfWidth), stepping sideways (dir -1 or 1) to centre toX at map height y, hits
	// something solid: the cell the player is in or the one at the hitbox's side (plan floor-queries), or a solid
	// sub-cell from the centre to that side, from the feet to the head (plan subcell-physics). The side the body leaves
	// is not asked: it can step out of one.
	[[nodiscard]] bool bodyBlocked(float toX, int dir, float y) const;
	// How far from the centre the player's feet and head meet solid sub-cells: the hitbox's half width, a little in.
	[[nodiscard]] float subcellReach() const;
	// The map y of the ceiling the player's head at feet height y meets (plan subcell-physics): a solid cell over the
	// centre, a solid sub-cell over the body; none if the head is free.
	[[nodiscard]] std::optional<float> ceilingAt(float y) const;
	// The ceiling trap (CeilingSpikes, CeilingDeath) the player's head at feet height y touches at ceilingAt's points,
	// NoObject for none (plan ceiling-spikes).
	[[nodiscard]] int ceilingTrapAt(float y) const;
	// A jump into a ceiling trap: a whole walk over the floor kind at the walk (TrapBell::walkHundredths, as a
	// monster's), through armour, the death trap's strong poison; it replaces the head bump (plan ceiling-spikes).
	void ceilingSpikesHit(int trap);
	// The player's body height against the shapes: PLAYER_BODY_HEIGHT, crouched CROUCH_HEIGHT_FACTOR of it.
	[[nodiscard]] float playerBodyHeight() const;
	// Nothing solid over the crouched body up to the standing head: no solid cell over the centre, no solid sub-cell
	// from side to side (a little in).
	[[nodiscard]] bool roomToStand() const;
	// The player stands on a sub-cell's top inside their cell, over the cell's bottom (a step): out of the water
	// basin and the sand under it.
	[[nodiscard]] bool playerOnStep() const;
	// Keys, gates, levers and rock falls (dungeon_mechanisms.cpp, drawn in dungeon_render_mechanisms.cpp).
	using Motion = CellMotion;
	int keysHeld = 0; // bit (colour - 1) per key picked up on this level
	int levelKeys = 0; // bit (colour - 1) per key the level has, picked up or not
	GateMotions gates; // the gates on the move (mechanisms.h)
	std::vector<Motion> fallingRocks; // c = 2 while in here, then 1 (fallen)
	int lockedHintMs = -1000000; // last "the gate is locked" message, to keep it from repeating every step
	int waterHintMs = -1000000; // last "too deep to jump" message
	int sprintPuffMs = -1000000; // last dust puff of a sprint step (plan sprint-toon-blur-and-extras)
	bool motionEffectsOn = true; // Options > Graphics > Motion effects (SetMotionEffects): the sprint's dust
	int wadeSoundMs = -1000000; // last splashing step, or step in sand
	void resetMechanisms();
	void updateMechanisms();
	// Opens the closed gates of the colour; a cut-scene shows the ones off the screen (startCutScene).
	void openGates(int colour);
	void bumpGate(int col, int row); // the player walked into a closed gate
	void startRockFall(int cell, bool byPlayer = false); // an armed rock fall cell was stepped into: the rumble starts
	static void landRock(Tile& t); // a falling rock fall's end: a rock becomes a Stone, a sand fall is Fallen
	void updateRocks();
	DartTraps dartTraps;
	void updateDartTraps();
	[[nodiscard]] bool plateLoaded(int col, int row) const; // the player or a heavy monster stands on it
	void pressPlate(int col, int row, bool byPlayer);
	void updateDarts();
	// Pressure plates (dungeon_plates.cpp, plan pressure-plate): each tick the weight on every plate; while all plates
	// of a colour carry theirs, its plate gates open, else they close (not on anyone standing in the gate's cell).
	PressurePlates plates; // which carry their weight (mechanisms.h)
	void updatePlates();
	[[nodiscard]] bool gateCellFree(int cell) const; // no one in the gate's cell: it may come down
	// Rhythm traps (dungeon_spikes.cpp, plans rhythm-traps, fire-jet-trap, poison-drip-trap): each wall spikes', fire
	// vent's and poison drip's live state from the clock; coming out, the spikes stab and shove out whoever stands in
	// the cell. A fire vent's jet burns whoever stands in its cell, a jump too (updateTraps); so does a poison drip's
	// falling venom poison them (updateDrips).
	void updateRhythmTraps();
	void stabWallSpikes(int col, int row);
	[[nodiscard]] std::optional<float> outOfSpikes(int col, int row, float centre, float half) const;
	// The fire vent's jet is on in a cell of row the body from left to right overlaps.
	[[nodiscard]] bool inFireJet(float left, float right, int row) const;
	// A cell of row the body from left to right overlaps has a fixture that is `on`.
	[[nodiscard]] bool overlapsFixture(float left, float right, int row, bool (*on)(const Fixture&)) const;
	// Poison drips (plan poison-drip-trap): the venom falls in a cell of row the body overlaps; a body's middle at x
	// stands in a puddle (DRIP_PUDDLE_HALF_WIDTH). Each poisons once per entry, the player and the monsters.
	[[nodiscard]] bool inFallingDrip(float left, float right, int row) const;
	[[nodiscard]] bool inPuddle(float x, int row) const;
	void updateDrips();
	bool playerPuddled = false, playerDropped = false; // the puddle's poison, the drop took this entry
	bool playerScorched = false; // the fire jet's line was told this entry (reset on leaving it)
	// Sunlight mirrors (dungeon_sun.cpp, plan sunlight-mirrors): the beams, the lit cells, the swings and flares.
	SunMirrors sun;
	// Traces the beams, lights the sun discs they fall on (their gates open), sets the sun burners in them on fire.
	void updateSunBeams();
	bool TurnMirror(); // interact on a mirror: a quarter turn clockwise; false if there is none
	// Dragging (dungeon_drag.cpp, plans mirror-drag, pressure-plate): what is held, its cell (map index, -1: none),
	// a body's monster slot, and the thing's centre's x less the player's, kept while it is dragged: it stays on that
	// side, the player faces it.
	enum class DragKind : std::uint8_t {
		Object, // a mirror or a stone (isDraggable): the object moves cell to cell
		Chest, // an emptied chest: its container (containers[]) moves, a dropped chest from then on
		Body, // a dead monster: its centre follows the player
	};
	struct Drag {
		DragKind kind = DragKind::Object;
		int cell = -1;
		int slot = -1; // monsters[] of a Body
		float offset = 0.f;
	} drag;
	// The thing let go: its cell, the map x it was drawn off its centre then, when (it slides back, dragShift).
	struct DragSettle {
		int cell = -1;
		float from = 0.f;
		int startMs = 0;
	} dragSettle;
	// A cell the player and a dragged thing may stand in: the flat corridor. Open and dry, no sand, no shape, on a
	// whole solid cell; for the thing (thing true) also no object, no fixture and no prop the player uses.
	[[nodiscard]] bool dragGround(int col, int row, bool thing) const;
	// The draggable thing in the player's cell, else in the next cell they face (facing -1 / +1) within DRAG_REACH: an
	// object first, then an emptied chest, then a body (its cell -1: none). offset unset.
	[[nodiscard]] Drag findDraggable(int facing) const;
	[[nodiscard]] Drag thingIn(int col, int row) const; // the draggable thing in the cell, findDraggable's order
	[[nodiscard]] bool stillHeld() const; // the thing held is still there (a mirror not gone, a body not cleared)
	[[nodiscard]] float heldCentre() const; // map x of the thing held: its cell's centre, a body's own
	// The walk moves the player to x: the thing comes along into its cell. False: refused (the player stays put).
	[[nodiscard]] bool dragTo(float x);
	void updateDrag(); // once a tick: lets go when the player can no longer drag (dead, in the air, held, ...)
	// Map x the dragged thing in cell is drawn off its cell's centre (0 for any other).
	[[nodiscard]] float dragShift(int cell) const;
	// The Mother of Webs (dungeon_webs.cpp, plan camel-spider-boss): awake, a douser (MonsterKind::douseCells) puts
	// out the torches and lamps near it on its row (DecorLayout::doused), a spinner (MonsterKind::webs) spins webs.
	void updateSpinner(const Monster& mon);
	// A hurt monster's pack call (CallRules, plan giant-rat-pack-call): the squeal, the journal, every monster that
	// answers within the radius is alerted.
	void callPack(const Monster& caller);
	int nextWebMs = 0; // GameClock: the spinner's next web (one boss per level)
	bool playerWebbed = false; // the player stood in a web on the last tick: the status line was told
	void updateWebs(); // the player caught in one, the fire vents' jets burn them, the cut and burnt ones go
	void burnWeb(int cell); // fire on the floor of cell (map index): its web burns
	// A blade's swing over the floor of row from fromX to toX (map x): the webs there are cut. How many.
	int cutWebs(float fromX, float toX, int row);
	[[nodiscard]] bool webFloor(int col, int row) const; // a web can lie there: dry floor, nothing in the cell
	Monster monsters[MAX_MONSTERS];
	[[nodiscard]] const Monster* nearestMonster() const; // the living one nearest the player; null if none
	// An empty slot, else the slot of a dead monster, else that of the farthest monster out of sight (sent back to
	// its spawn tile); null if none.
	[[nodiscard]] Monster* freeMonsterSlot();
	// The level's boss fight (at most one boss per level). A save keeps the hurt monsters' health and the fight's
	// counters, not where they stand (plan save-live-monsters): after a load each comes out of its spawn tile again.
	struct BossFight {
		int slot = -1; // monsters[] index of the boss, -1: none in play
		int summoned = 0; // minions summoned this fight, after the first ones
		int nextSummonMs = 0; // GameClock time of the next summon
		int firstLeft = 0; // hatchers: first minions still to come (the egg clusters spawn in view after the boss)
		int hatched =
			0; // minions hatched, out of a wall or dropped so far: picks the kind (BossRules::smallMinion), the nest
	} bossFight;
	// A save's hurt monsters and boss fight (plan save-live-monsters), until their spawn tile comes into view.
	struct SavedMonster {
		int col = 0, row = 0; // spawn tile
		int hp = 0;
	};
	std::vector<SavedMonster> savedMonsters;
	struct SavedBossFight {
		int summoned = 0, hatched = 0, firstLeft = 0;
		int minions = 0; // living minions at the save: they come back as first minions, no more than minAlive
	};
	std::optional<SavedBossFight> savedBossFight;
	void dumpMonsters(std::ostream& f) const;
	bool loadMonsters(std::istream& f); // false: a broken list
	void startBossFight(int slot); // the boss appeared: its first minions with it (a saved fight: as it was)
	void updateBoss(); // summons while it lives; its death opens the boss gates, for good
	bool summonMinion(const Monster& boss);
	// Summon::Wall: the wall cell on the player's row the way they look, with a free cell in front of it the minion
	// steps onto, not the player's own; else the same behind them. One on screen first (WALL_SUMMON_SIGHT), then any.
	// False: none (the row ends in no wall).
	bool wallSummonCell(int& outWall, int& outCell, int& outRow) const;
	int playerFacing = -1; // the way the player last walked or turned (Move): -1 left (as the game starts), +1 right
	int walkDir = -1; // the way the player last stepped (Move), turned or not: dragging, pulling or pushing
	// The player's fall of `rows` ended in their cell: the thud, and a puff on a heap or sand (plan soft-landing).
	void playerLanded(float rows);
	// Map x a burrower comes up at: BURROW_BEHIND past the player, away from where it is; else as far in front of
	// them. False: neither is floor it can stand on.
	[[nodiscard]] bool burrowTarget(const Monster& mon, float& outX) const;
	// Map x a sand burrower (MonsterKind::sandBurrow) in sand comes up at: under the player on its row within range
	// tiles, through sand all the way, or past tiles beyond them if that is sand too; for a lurker (edge) at the end of
	// its sand nearest a player off the sand too. False: none.
	[[nodiscard]] bool sandTarget(const Monster& mon, float range, bool edge, float past, float& outX) const;
	Projectiles projectiles; // missiles, monster shots, dung balls (projectiles.h)
	Hazards hazards; // webs, blazes, chest gas, holes, puffs, landings, snuffs (hazards.h)
	using ChestGas = Hazards::ChestGas;
	using Web = Hazards::Web;
	using Blaze = Hazards::Blaze;
	using Hole = Hazards::Hole;
	using Landing = Hazards::Landing;
	void updateMissiles();
	void shootAtPlayer(const Monster& mon, float x, float y);
	void aimShot(MonsterShot& v) const; // at the player as they stand: the chest, a big coal the feet
	void updateShots();
	bool playerBlazed = false; // the player stood in a blaze on the last tick: the status line was told
	void blazeAt(float x, float y, int ms); // the floor of the cell under (x, y) burns, unless it is wet or no floor
	void updateBlazes();
	[[nodiscard]] bool blazeFloor(int col, int row) const; // a dry floor cell
	// The last power hit (plan skill-power-hit): where it landed (map units), when, its charge 0..1. Its spark and the
	// view's shake (PowerHitShake) grow with the charge.
	struct PowerHitMark {
		float x = 0.f, y = 0.f;
		int atMs = -1;
		float power = 0.f;
	};
	PowerHitMark powerHit;
	[[nodiscard]] bool clearRow(float fromX, float toX, int row) const; // no solid cell between the two on row
	bool updateRoller(Monster& mon); // the windup and the kick; true: busy with it, no walk or bite
	void updateBalls();
	void breakBall(DungBall& ball, bool inWater);

	void dropChest(const Monster& mon, ItemKind weapon, bool cursed = false); // RollKillDrop; cursed: placeDroppedChest
	void updateTraps();
	void updateMonsterPoison(); // the running poison of each monster; the player's poison kill is theirs
	void updateBurning(); // the player's and each monster's fire (plan fire-damage); a monster's burning kill rewards
	// The near edge (at mid height) of the nearest living monster ahead (dir -1 / +1) within range tiles of (x, y) the
	// bow can reach, or false.
	bool aimTarget(float x, float y, int dir, float range, float& outX, float& outY) const;
	GateTravel travel; // through a gate (gate_travel.h); the world stands still meanwhile
	void startTravel(bool exitGate, bool rings = false);
	void updateTravel(); // the jump under the faded frame, the next phase
	StargateDial dial; // the Stargate dialled (stargate_dial.h, dungeon_stargate.cpp)
	void startDial();
	void pressDialDevice();
	void weighHeart(); // the scales of the judgment (plan weighing-of-the-heart)
	void updateDial(); // the chevrons' sounds, the failing seventh's message and reward
	// Cut-scenes (cut_scene.h, dungeon_cutscene.cpp): the camera goes to the gates a lever or a boss's death opened off
	// the screen. The world stands still meanwhile, but for the gates.
	CutScene cutScene;
	// The cut-scene for the gates just opened (cells): each starts to open as the camera comes to it.
	void startCutScene(const std::vector<int>& opened);
	void updateCutScene();
	// The monster tiles in the gameplay window (ViewWindow, 10 x 6 cells round the player) spawn their monsters.
	void spawnInView();
	// Each tick: drops the holes, sprays, puffs, landings and pours that ran out, starts the pour of a sand fall
	// whose warning ended. The draw only reads them (plan render-reads-sim).
	void updateEffects();

  public:
	// Feet at (x, y) in map units, half wide (a box; 0 a point), touch a spike or death trap's hitbox:
	// TRAP_HITBOX_X/Y_SCALE x the trap's scale round the tile's bottom centre. The box is TRAP_BOX_FORGIVE narrower on
	// each side (plan trap-player-box), so grazing an edge doesn't count.
	// The trap it stands in: its tile type (Spike or Death) and x's offset from its middle in half widths (-1 to 1).
	struct TrapContact {
		int type;
		float offset;
		float centre; // its middle's map x
		float half; // its hitbox's half width, widened by the box: the centre's span in it
	};
	[[nodiscard]] std::optional<TrapContact> inTrap(float x, float y, float half) const;
	// A body on foot at map x in a spike or death trap this tick (here; none: out of it), the player and a monster by
	// one rule (plan same-hazards-for-all): the bell (TrapBell) over the way it covered since the tick before at its
	// pace (tiles a second; none: the way this tick), times the trap's crossing, plus TrapHurt's ramp. Hundredths of a
	// HP, before its trap share. Airborne (a jump over it) only the ramp: a graze.
	[[nodiscard]] static int TrapWalkHundredths(
		TrapHurt& hurt,
		float x,
		const std::optional<TrapContact>& here,
		int maxHp,
		std::optional<float> pace,
		bool airborne
	);
	// The death trap's venom, once per entry (TrapHurt::venomed): true when it is due now.
	[[nodiscard]] static bool TrapVenomDue(TrapHurt& hurt, const std::optional<TrapContact>& here, int hundredths);
	// A spike or death trap's bleed roll, once per entry that hurt (TrapHurt::bleedRolled, plan bleeding): its source
	// when due now.
	[[nodiscard]] static std::optional<BleedRoll>
	TrapBleedDue(TrapHurt& hurt, const std::optional<TrapContact>& here, int hundredths);
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
	[[nodiscard]] int Zone() const { return meta.zone; } // the ZoneId, or NO_ZONE (LevelMeta)
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
	[[nodiscard]] const std::vector<DungBall>& Balls() const { return projectiles.balls; }
	// The ankh's choice made (plan ending-choice): the run is won, with that ending; Renounced breaks the ankh. Once.
	void Win(RunEnding ending);
	void ClearWin() { won = false; } // a new game or a scenario level
	// Scenario `peace` (make paths): the monsters stand down as after the win, so their holds, dazes and curses
	// do not stop a script that plays the checker's path (the checker does not model them). Kept over level loads.
	void SetPeace(bool on) { peace = on; }
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
	// The treasure of (col, row) was taken: its open container stays (the draft map's pale mark).
	[[nodiscard]] bool ContainerEmptied(int col, int row) const {
		return IsInBounds(col, row) && containers[MapIndex(col, row)].emptied;
	}
	// The look of the container in (col, row), and how much of a cursed coffin's text shows there (coffinTell).
	[[nodiscard]] TreasureLook LookAt(int col, int row) const { return treasureLook(MapIndex(col, row)); }
	[[nodiscard]] float CoffinTellAt(int col, int row) const { return coffinTell(MapIndex(col, row)); }
	// Scenario `container`: the treasure in (col, row) looks like `kind` (CONTAINER_NAMES) in STANCES[stance],
	// whatever the level's scatter picked; a coffin inscribed or not. False if the cell is outside the level or the
	// stance unknown.
	bool PlaceContainer(int col, int row, int kind, int stance, bool inscribed = false);
	// The highest decoration tier (decor.h) among the level's props, decals and surfaces; -1 for a bare level.
	[[nodiscard]] int DecorTierUsed() const;
	void Update();
	void AnimateMonsters(); // once a tick, after Update: every active monster (Monster::Animate)
	bool Move(float dirX, float dirY); // false: blocked or held (PlayerHeldBy), the player did not move
	// Dragging (plans mirror-drag, pressure-plate): the drag key takes hold of a draggable thing (a mirror, a stone, an
	// emptied chest, a monster's body: findDraggable) in the player's cell or the next one they face (facing -1 / +1),
	// or lets go. Held, the player crouches facing it and walks backwards
	// pulling it (or forwards pushing it) at DRAG_WALK_FACTOR of the crouched walk, the thing a cell at a time; only on
	// the flat corridor (dragGround). True: took hold.
	bool ToggleDrag(int facing);
	void LetGo(); // lets go of the thing held, if any
	[[nodiscard]] bool Dragging() const { return drag.cell >= 0; }
	// Dragging: the way the player looks, at the thing (-1 / +1). Else 0.
	[[nodiscard]] int DragFacing() const;
	// Dragging, the last step went away from the thing held (plan drag-animation: the pulling clip, else pushing).
	[[nodiscard]] bool DragPulling() const { return Dragging() && walkDir != DragFacing(); }
	// Not dragging, a draggable thing within reach the way the player faces (the first hint).
	// The weight on the pressure plate in (col, row) (plan pressure-plate): the player, monsters and bodies over it,
	// the stone, emptied chest or mirror in its cell.
	[[nodiscard]] int WeightOn(int col, int row) const;
	[[nodiscard]] bool PlateDown(int col, int row) const {
		return IsInBounds(col, row) && plates.Down(MapIndex(col, row));
	}
	[[nodiscard]] bool PlayerCanDrag(int facing) const { return !Dragging() && findDraggable(facing).cell >= 0; }
	// Standing in half water (not in the air): the walk slows to WADE_SPEED_FACTOR (WADE_SPEED_FACTOR_HAPI with the
	// Favour of Hapi skill, plan skill-blessing-of-hapi).
	[[nodiscard]] bool PlayerWading() const;
	// The walk's speed factor: wading, sand, a web; dazed (plan giant-bat-screech) DAZE_WALK_FACTOR on top.
	[[nodiscard]] float PlayerWalkFactor() const;
	// A walk key's step factor: PlayerWalkFactor times the player's own (PlayerStats::WalkFactor: slowness, rings, the
	// Speed of Mafdet on firm ground; none of them in the air).
	[[nodiscard]] float PlayerStepFactor() const;
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
	// A walk step may sprint: not wading (without the Favour or the Blessing of Hapi), not in sand, not dazed, not
	// crouched.
	[[nodiscard]] bool PlayerCanSprint() const;
	// Wading with the skill and the potion: the sprint drains the stamina at WADE_SPRINT_DRAIN_BOTH.
	[[nodiscard]] bool PlayerHalvedWadeDrain() const {
		return PlayerWading() && PlayerHasHapi() && PlayerWaterBlessed();
	}
	// False while wading (and without the Favour of Hapi or the potion): then the player is told why (at most every
	// WATER_JUMP_HINT_MS).
	bool JumpAllowed();
	// No new attack while hanging on a ladder (PlayerOnLadder): both hands are on the rungs, nor in a crocodile's jaws
	// (PlayerHeldBy), nor crouched (plan crouch). A swing already under way finishes. tell: say why ("Hands on the
	// rungs"), once per press of the key.
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
	// The map y of the floor the player at (x, y) stands on or falls onto (floorBelow); floor(y) where there is none
	// (plan floor-queries).
	[[nodiscard]] float floorUnder(float x, float y) const;
	// The floor under the player at (x, y), at most a cell under y's (plan subcell-physics): the top of the cell under
	// y's if it is a floor (isFloor, under the body's centre x), or of a solid sub-cell at or under y, under any part
	// of the body (a foot on a step carries the player). None over a shaft.
	[[nodiscard]] std::optional<float> floorBelow(float x, float y) const;
	// A solid sub-cell of a shaped cell at map (x, y) (plan subcell-physics); whole solid cells are isSolidTile's.
	[[nodiscard]] bool subcellAt(float x, float y) const;
	// How far the player is above floorUnder: in a jump, a fall, up a ladder (plan floor-queries).
	[[nodiscard]] float heightOverFloor() const;
	// World units the player has sunk into quicksand: down to QUICKSAND_SINK_DEPTH tiles as they get stuck.
	[[nodiscard]] float QuicksandSink() const;
	// World units the player is pulled down into the sand, held by a swallower (HoldRules::swallow, plan worm-boss).
	[[nodiscard]] float SwallowSink() const;
	// On a ladder, within reach of it and off the floor: the player hangs on it (climb clip, back to the camera).
	// Walking into a ladder cell from the side keeps the walk / idle clip until climbing pulls the player over. Not
	// after a daze made them let go (lostGrip).
	[[nodiscard]] bool PlayerOnLadder() const;
	// First hints (plan controls-help): a ladder in the player's cell or the one under their feet.
	[[nodiscard]] bool PlayerAtLadder() const;
	// First hints: standing on a floor, facing dir (-1 / +1) a jump within a step: a gap of a cell or two with a floor
	// beyond it, or a wall one cell high with room over it and the player.
	[[nodiscard]] bool PlayerFacesJump(int dir) const;
	// Climb clip phase: one cycle per tile climbed, running on (at half rate) while moving sideways on the ladder.
	[[nodiscard]] float ClimbPhase() const;
	int Type(float x, float y);
	void getC(float& outX, float& outY);
	// The player's melee attack: hits the nearest monster in range ahead (dir -1 / +1). False: nothing in reach.
	// reach in tiles (Item::Reach). power: a power hit's charge 0..1 (plan skill-power-hit), its thump, spark and
	// shake.
	bool AttackNearest(int damage, const DamageMix& mix, float reach, int dir, float power = 0.f, bool exact = false);
	// 0..1: how hard the view shakes after a power hit, fading over POWER_SHAKE_MS; 0 with none.
	[[nodiscard]] float PowerHitShake() const;
	// A ranged weapon shoots: the missile leaves the hand, height above the player's feet in tiles, facing dir
	// (-1 / +1), aimed at a monster up to aimRange tiles ahead.
	void Shoot(
		MissileKind kind,
		int damage,
		const DamageMix& mix,
		int dir,
		float height,
		float aimRange,
		bool exact = false
	);
	// Why the ranged weapon's shot facing dir would not leave (plan ranged-weaker-than-melee): Empty, none of its shots
	// left (ItemBag::ShotsLeft); TooClose, a monster the bow would aim at is nearer than its minimum range (between
	// the hitboxes, weaponMinRange; a wall, a lever or a target never blocks). None for a melee weapon.
	enum class ShotRefusal : std::uint8_t { None, Empty, TooClose };
	[[nodiscard]] ShotRefusal ShotRefused(ItemKind weapon, int dir) const;
	// Tells a refused shot, at most every SHOT_REFUSAL_REPEAT_MS: the dry click (Empty), or the dull knock and "Too
	// close to shoot" (TooClose); the HUD's weapon slot blinks (Player::shotRefusedMs).
	void RefuseShot(ShotRefusal why);
	// The ranged weapon's shot at the draw's end: Shoot, one of its shots spent. A monster that came too close during
	// the draw stops it (RefuseShot, nothing spent): false.
	bool ShootWeapon(ItemKind weapon, int damage, int dir, float height, float aimRange);
	// The interact key: does what InteractVerb says, nothing on None.
	void Interact();
	// What the interact key does in the player's cell, in this order (plan wall-layer, interact-prompt): the chest or a
	// mimic in ambush (Open; Take once the chest's clip has played, nothing while it plays), then the fixture on the
	// back wall (Pull a lever not pulled yet, Read or Copy an inscription), the brazier with a bow (Light), and the
	// gate last, as it may take the player away (Answer a riddle gate, Enter the exit or a teleporter not in the air,
	// Take the ankh). Keys are picked up on touch: None.
	[[nodiscard]] Interaction InteractVerb() const;
	// The god whose shrine stands in the player's cell (Pray's reach): offerings and the magic items' upgrades there
	// (plan shrine-offerings). None elsewhere.
	[[nodiscard]] std::optional<God> ShrineHere() const;
	// The glint over an unread inscription in (col, row) (plan interact-prompt), 0 or 1: 1 not in the journal yet; 0
	// once read (copied or translated; plan read-inscription-no-glint), too worn to read, or no inscription.
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
	[[nodiscard]] bool Travelling() const { return travel.Active(); }
	[[nodiscard]] float TravelFade() const {
		return travel.Fade();
	} // 0..1: how far the view is faded into the gate colour
	[[nodiscard]] bool TravelToExit() const { return travel.ToExit(); }
	[[nodiscard]] bool TravelByRings() const { return travel.ByRings(); }
	// The Stargate in (col, row) (plan stargate-chamber): how bright its chevron k (0 to STARGATE_CHEVRONS - 1, the
	// seventh at the top) glows, 0..1, and the turn of its inner track in degrees (counter-clockwise as seen).
	[[nodiscard]] float ChevronGlow(int col, int row, int chevron) const {
		return dial.ChevronGlow(MapIndex(col, row), chevron);
	}
	[[nodiscard]] float StargateTrackDeg(int col, int row) const { return dial.TrackDeg(MapIndex(col, row)); }
	[[nodiscard]] bool Dialling() const { return dial.Dialling(); }
	// Ring k's way down out of the ceiling while a ring platform carries the player (plan ring-transporter), 0 up in
	// its housing to 1 at rest round the player; 0 when not travelling by rings. Ring 0 drops first, to the bottom.
	[[nodiscard]] float RingDrop(int ring) const { return travel.RingDrop(ring); }
	// Pushes the player tiles along the row, dir -1 / +1, sliding (a storm roar). Not while held.
	void ShovePlayer(int dir, float tiles);
	// 0..1: how thick the sand over the screen is (a storm roar), fading out.
	[[nodiscard]] float SandHaze() const;
	// 0..1: how thick the sandstorm hall's haze is round the player (plan sandstorm-halls): the fog closes in.
	[[nodiscard]] float StormHaze() const { return storm; }
	// 0..1: the share of the other sounds' volume the wind of a gust near the player takes now.
	[[nodiscard]] float SoundMuffle() const { return muffle; }
	[[nodiscard]] int DustPuffs() const {
		return static_cast<int>(hazards.puffs.size());
	} // steps' dust still in the air
	// The landing puffs still in the air (plan soft-landing), the newest one's heap (-1 for none).
	[[nodiscard]] int LandingPuffs() const { return static_cast<int>(hazards.landings.size()); }
	[[nodiscard]] int LastLandingHeap() const { return hazards.landings.empty() ? -1 : hazards.landings.back().heap; }
	void SetMotionEffects(bool on) { motionEffectsOn = on; }
	void SetCutScenes(bool on) { cutScene.SetEnabled(on); } // off: remote gates open unseen
	[[nodiscard]] bool CutScenePlaying() const { return cutScene.Playing(); }
	[[nodiscard]] CutShot CutSceneShot() const { return cutScene.Shot(); }
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
	// The player's chest is in a screech's cone (plan giant-bat-screech): ahead along the flyer's way, within its range
	// and half its angle off that way, no wall on the line between.
	[[nodiscard]] bool InScreechCone(const Monster& mon, const ScreechRules& rules) const;
	// Every slot (MAX_MONSTERS), active or not (Monster::Active).
	[[nodiscard]] const Monster* Monsters() const { return monsters; }
	[[nodiscard]] int NearestMonsterHealth() const; // of the living monster nearest the player, 0 if none
	[[nodiscard]] int NearestMonsterPoison() const; // its running poison tiers (Poison::Mask), 0 if none
	[[nodiscard]] int NearestMonsterBleed() const; // its running bleed tiers (Bleeding::Mask), 0 if none
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
	// Terrain affinity (plan terrain-affinity). The terrains of (col, row) for a monster on the ground (ground false: a
	// flyer, over water, sand and webs). A fire vent's cell is fire while its jet burns, or always (anyVent: the
	// avoidance, monsters know the trap).
	[[nodiscard]] Terrains TerrainsAt(int col, int row, bool ground = true, bool anyVent = false) const;
	// The torch or lamp in (col, row) was put out (DecorLayout::doused, plan camel-spider-boss).
	[[nodiscard]] bool Doused(int col, int row) const {
		return IsInBounds(col, row) && decoration.doused[MapIndex(col, row)];
	}
	// A whole web lies over the floor of (col, row) (plan camel-spider-boss).
	[[nodiscard]] bool WebAt(int col, int row) const {
		return IsInBounds(col, row) && hazards.WebAt(MapIndex(col, row)) >= 0;
	}
	// The player stands in a web (not in the air): hostile terrain, the walk slows to WEB_SLOW_FACTOR
	// (PlayerWalkFactor).
	[[nodiscard]] bool PlayerInWeb() const;
	// The terrains of the player's cell (TerrainsAt; in the air, none of the ground's) and what the player's affinity
	// (PlayerStats::TerrainNative, TerrainHostile) makes of them (plan same-hazards-for-all): the walk's factor and
	// the HUD's mood card.
	[[nodiscard]] Terrains PlayerTerrains() const;
	[[nodiscard]] TerrainMood PlayerTerrainMood() const;
	[[nodiscard]] TerrainFactors PlayerTerrainFactors() const;
	// The floor of (col, row) burns (Babi's big coal, plan babi-boss).
	[[nodiscard]] bool BlazeAt(int col, int row) const;
	[[nodiscard]] bool PlayerInBlaze() const; // the player stands in a burning floor cell, not in the air
	// Monster shots in flight or waiting in a volley (plan babi-boss); big: only the big coals.
	[[nodiscard]] int ShotCount(bool big = false) const { return projectiles.ShotCount(big); }
	// Unit tests: the map x where the missiles stuck in a wall, the floor or a solid sub-cell are.
	[[nodiscard]] std::vector<float> StuckMissileXs() const { return projectiles.StuckMissileXs(); }
	// A beam of sunlight runs through (col, row) now (plan sunlight-mirrors); it counts as light for Lit too.
	[[nodiscard]] bool Sunlit(int col, int row) const { return IsInBounds(col, row) && sun.Sunlit(MapIndex(col, row)); }
	// A minion's kill: 1 XP while its boss lives (no farming), half its type's xp after the boss died.
	[[nodiscard]] int MinionXP(int xp) const { return bossFight.slot >= 0 ? 1 : xp / 2; }
	// Draft map: the cells within EXPLORE_RADIUS of every tile the player stood on.
	static constexpr int EXPLORE_RADIUS = 1; // cells to each side: a 3 x 3 square
	[[nodiscard]] bool Explored(int col, int row) const { return IsInBounds(col, row) && explored[MapIndex(col, row)]; }
	[[nodiscard]] Tile Cell(int col, int row) const { return MapAt(col, row); }
	void Dump(std::ostream& f);
	bool LoadDump(std::istream& f);
	// Props, decals, torches, ladders and surfaces, seeded by the level's file name, from the decoration tiers its
	// depth unlocks (decor.h: the campaign level, DECOR_DEPTH_ALL outside the campaign).
	void scatterDecorations(const char* levelName, int depth);
	[[nodiscard]] bool bossCoffin(int i, int j) const; // a coffin for the boss's minions stands there
};

#endif
