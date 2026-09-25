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

enum class ModelState { Die = 0, Walk = 1, Attack = 2, Jump = 3 };

// Playback of every clip, indexed by ModelState.
using MonsterAnimations = std::array<AnimPlayback, 4>;

class monster {
  private:
	std::unique_ptr<AnimatedCartoonModel> walk;
	std::unique_ptr<AnimatedCartoonModel> attack;
	std::unique_ptr<AnimatedCartoonModel> die;
	std::unique_ptr<AnimatedCartoonModel> jumpAnim; // optional <name>_jump.md3 (player only); falls back to walk
	AnimatedCartoonModel* model;
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
	bool loadModel(const char filename[], Textura& texture, Textura& nullT, bool compile = 1);
	void setCords(float nX, float nY);
	float rotA;
	float scale;
	// AI functions
	int attackDirection();
	bool getHit(int dmg);
	bool Alive();
	int Seek();
	void Attack();
	void Reanimate();
	void GetCords(float& xx, float& yy);
	bool Nearby(float xx, float yy, int range);
	void changeMDL(int id);
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
};

#endif
