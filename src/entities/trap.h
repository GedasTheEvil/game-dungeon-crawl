#ifndef TRAP_H
#define TRAP_H
#include <memory>
#include "../graphics/animated_model.h"
#include "../graphics/textures.h"
#include "../core/timer.h"

class Trap {
  private:
	std::unique_ptr<AnimatedModel> mdl;
	float tileX;
	float tileY;
	Texture tex;
	std::unique_ptr<Timer> Hurt_timer;

  public:
	float scale;
	float* dungeonCamY;
	float* dungeonCamX;

	Trap();
	~Trap();
	void Show();
	void Hurt();
	void setCords(float nX, float nY);
	bool loadModel(const char filename[], Texture& texture, bool compile = true);
	void debugText();
};

#endif
