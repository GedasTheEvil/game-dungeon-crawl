#ifndef TrapsH
#define TrapsH

#include <memory>
#include "../graphics/shader.h"
#include "../graphics/textures.h"
#include "../core/timer.h"

class trap {
  private:
	std::unique_ptr<AnimatedCartoonModel> mdl;
	float tileX;
	float tileY;
	Textura tex;
	std::unique_ptr<timer> Hurt_timer;

  public:
	float scale;
	float* dungeonCamY;
	float* dungeonCamX;

	trap();
	~trap();
	void Show();
	void Hurt();
	void setCords(float nX, float nY);
	bool loadModel(const char filename[], Textura& texture, bool compile = 1);
	void debugText();
};

#endif
