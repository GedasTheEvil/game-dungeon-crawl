#ifndef ItemH
#define ItemH

#include <memory>
#include "../graphics/shader.h"
#include "../graphics/textures.h"

class item {
  private:
	bool in_inventory;
	std::unique_ptr<AnimatedCartoonModel> mdl;
	float x;
	float y;
	Textura tex;
	bool loaded;

  public:
	int damage, range, type, heal, armor, hp;
	float rotA;
	float scale;
	void Draw();
	bool getPickedUp();
	item();
	~item();
	bool loadModel(const char filename[], Textura& texture, bool compile = 1);
};

#endif
