#ifndef TRAP_H
#define TRAP_H
#include <memory>
#include "../graphics/animated_model.h"
#include "../graphics/textures.h"
#include "../core/timer.h"
#include "../core/gameplay_config.h"

class Trap {
  private:
	std::unique_ptr<AnimatedModel> mdl;
	float tileX;
	float tileY;
	Texture tex;
	Timer Hurt_timer{TRAP_HURT_INTERVAL_MS};

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
