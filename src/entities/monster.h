#ifndef MonsterH
#define MonsterH

#include "../graphics/shader.h"
// #include "../world/Dungeon.h"
#include "../graphics/textures.h"
#include "../graphics/particles.h"
#include "../core/sound.h"
#include "../core/timer.h"
#include <array>
#include <memory>
#include <vector>

// Animation clips. Each is one file, see ClipFile; a missing optional clip shows the reference clip.
enum class ModelState { Die = 0, Idle = 1, Move = 2, Attack = 3, Jump = 4, Climb = 5 };
constexpr int MODEL_STATE_COUNT = 6;

// Playback of every clip, indexed by ModelState.
using MonsterAnimations = std::array<AnimPlayback, MODEL_STATE_COUNT>;

// Models/<category>/<name><suffix>.md3 for one clip. The first entry of a list is the reference clip:
// it is required, sets the normalization of all clips (frame 0) and stands in for missing optional ones.
struct ClipFile {
	ModelState state;
	const char* suffix;
	bool required;
	bool loop; // false: plays once and holds the last frame
};
using ClipFiles = std::vector<ClipFile>;
// Monsters: <name>.md3 move, _att attack, _die die, optional _idle (e.g. a bat on the ceiling).
inline const ClipFiles MONSTER_CLIPS = {{ModelState::Move, "", true, true},
										{ModelState::Attack, "_att", true, true},
										{ModelState::Die, "_die", true, false},
										{ModelState::Idle, "_idle", false, true}};
// The player: <name>.md3 idle (standing), _walk, _die, optional _jump and _climb. No attack clip (the weapon swings).
inline const ClipFiles PLAYER_CLIPS = {{ModelState::Idle, "", true, true},
									   {ModelState::Move, "_walk", true, true},
									   {ModelState::Die, "_die", true, false},
									   {ModelState::Jump, "_jump", false, false},
									   {ModelState::Climb, "_climb", false, true}};

// Flying monsters (bats): hang on the ceiling until the player comes near, then swoop through him,
// biting on the way, fly on, turn and come back. Kept per dungeon token, like the clip playback.
enum class FlightPhase { Roost, Swoop, Return };
struct Flight {
	FlightPhase phase = FlightPhase::Roost;
	int dir = 1;		 // +1 flying right, -1 left
	float lift = -1.f;	 // world units from the floor to the model origin; < 0: not placed yet (on the ceiling)
	float fall = 0.f;	 // falling speed after death, world units per second
	bool bitten = false; // this pass has bitten already
	int attackUntilMs = 0;
	int lastMs = -1; // GameClock time of the last update
};

class monster {
  private:
	std::array<std::unique_ptr<AnimatedCartoonModel>, MODEL_STATE_COUNT> clips; // by ModelState, nullptr if no file
	ModelState reference = ModelState::Move;									// first clip of the ClipFiles
	AnimatedCartoonModel* model;
	// Frame 0 extents in model units: a flyer hangs from the ceiling by the idle clip's top.
	float referenceTop = 1.f;
	float idleBottom = 0.f, idleTop = 1.f;
	float mapX;
	float mapY;
	int speed;
	int damage;
	int XP;
	int stat;
	int facing_dir;
	Textura nullTexture, tex;
	std::unique_ptr<ParSys> ownBlood;
	ParSys* blood; // ownBlood, or the particles of the dungeon token being processed
	rgb bloodColour = {0.7f, 0.1f, 0.1f};
	std::unique_ptr<timer> walk_timer;
	ModelState currentState;
	void applyModelState(ModelState state);
	void selectModel(ModelState state); // no reset: used to restore a token's animation
	[[nodiscard]] AnimatedCartoonModel* clip(ModelState state) const;

  public:
	std::unique_ptr<timer> Att_timer;
	Sound die_s, att_s;

	int health;
	int maxHealth;

	float* dungeonCamY;
	float* dungeonCamX;

	float tileOriginX;
	float tileOriginY;

	monster();
	monster(float dx, float dy);
	monster(float nX, float nY, int nSpeed, int nHP, int nDamage, int nXP);
	~monster();
	bool Draw();
	bool loadModel(const char filename[], Textura& texture, Textura& nullT, bool compile = 1,
				   const ClipFiles& files = MONSTER_CLIPS);
	void setCords(float nX, float nY);
	float rotA;
	float scale;
	float depthOffset = 0.f; // the player only: moved towards the back wall (world units) while climbing
	// Climb clip at phase 0..1 of its cycle, set by the caller instead of the clock (no-op without the file).
	void showClimb(float phase);
	[[nodiscard]] bool climbing() const { return currentState == ModelState::Climb; }
	// AI functions
	int attackDirection();
	bool getHit(int dmg);
	bool Alive();
	int Seek();
	void Attack();
	// Flyers: one step of the bat behaviour (see Flight); wallAhead: the cell in front of it blocks the flight.
	bool flies = false;
	Flight flight;
	void Fly(bool wallAhead);
	[[nodiscard]] float flightProbeX() const; // map x the flyer checks for walls
	void Reanimate();
	void GetCords(float& xx, float& yy);
	bool Nearby(float xx, float yy, int range);
	void setModelState(ModelState state);
	int Model_state();
	void setFacingDir(int dir);
	int FacingDir();
	void setBloodColor(float r, float g, float b);
	// Monsters of one type share one object, so each dungeon token owns its own blood particles.
	void initBlood(ParSys& tokenBlood) const;
	void useBlood(ParSys* tokenBlood);
	// Monsters of one type share one object, so each dungeon token keeps its own clip playback.
	[[nodiscard]] MonsterAnimations spawnAnimations() const;
	void restoreAnimations(int state, const MonsterAnimations& animations);
	[[nodiscard]] MonsterAnimations animations() const;
	float healthRatio() const;
};

struct monsterToken {
	float mapX, mapY;
	int orX, orY; // origin in map field
	int type;
	int HP;
	monster* m;
	std::unique_ptr<ParSys> blood;
	std::unique_ptr<timer> t;
	std::unique_ptr<timer> at;
	int state;
	int facing_dir;
	MonsterAnimations anim;
	Flight flight;
};

#endif
