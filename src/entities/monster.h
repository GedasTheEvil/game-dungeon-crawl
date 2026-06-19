#ifndef MonsterH
#define MonsterH

#include "../graphics/shader.h"
// #include "../world/Dungeon.h"
#include "../graphics/textures.h"
#include "../graphics/particles.h"
#include "../core/sound.h"
#include "../core/timer.h"
#include <memory>

class monster {
  private:
	std::unique_ptr<AnimatedCartoonModel> walk;
	std::unique_ptr<AnimatedCartoonModel> attack;
	std::unique_ptr<AnimatedCartoonModel> die;
	AnimatedCartoonModel* model;
	float x; // kiek nuejo nuo pradzios
	float y;
	int speed;
	int damage;
	int XP;
	int stat;
	int facing_dir;
	Textura nullTexture, tex;
	std::unique_ptr<ParSys> blood;
	std::unique_ptr<timer> walk_timer;

  public:
	std::unique_ptr<timer> Att_timer;
	Sound die_s, att_s;

	int health;
	int maxHealth;

	float* dungeonY; // ne , ne isvestine :D. Kordinates pozemio
	float* dungeonX;

	float X; // kordinates kur monstras gyvena pozemyje
	float Y;

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
	void setModel(int state);
	void setFacingDir(int dir);
	int FacingDir();
	void setBloodColor(float r, float g, float b);
	float healthRatio() const;
};

struct monsterToken {
	float x, y;
	int orX, orY; // origin in map field
	int type;
	int HP;
	monster* m;
	std::unique_ptr<timer> t;
	std::unique_ptr<timer> at;
	int state;
	int facing_dir;
	int frame;
};

#endif
