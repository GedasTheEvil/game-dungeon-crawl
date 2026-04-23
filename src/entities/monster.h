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
	CartoonANI* walk;
	CartoonANI* attack;
	CartoonANI* die;
	CartoonANI* mdl;
	float x; // kiek nuejo nuo pradzios
	float y;
	int speed;
	int damage;
	int XP;
	int stat;
	Textura TNull, tex;
	ParSys* blood;
	timer* walk_timer;

  public:
	timer* Att_timer;
	Sound die_s, att_s;

	int HP;
	int MaxHP;

	float* DY; // ne , ne isvestine :D. Kordinates pozemio
	float* DX;

	float X; // kordinates kur monstras gyvena pozemyje
	float Y;

	monster();
	monster(float dx, float dy);
	monster(float nX, float nY, int nSpeed, int nHP, int nDamage, int nXP);
	~monster();
	bool Draw();
	bool LoadMDL(const char filename[], Textura& texture, Textura& nullT, bool compile = 1);
	void setCords(float nX, float nY);
	float rotA;
	float scale;
	// AI functions
	int AtDir();
	bool getHit(int dmg);
	bool Alive();
	int Seek();
	// 	   void Attack(monster *target);//obsolete for attacking player, but can be used to attack other monsters,
	// right?
	void Attack();
	void Reanimate();
	void GetCords(float& xx, float& yy);
	bool Nearby(float xx, float yy, int range);
	void changeMDL(int id);
	int Model_state();
	void setModel(int state);
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
	int frame;
};

#endif
