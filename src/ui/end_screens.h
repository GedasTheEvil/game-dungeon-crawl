#ifndef END_SCREENS_H
#define END_SCREENS_H
#include "../graphics/textures.h"
#include "../core/timer.h"

class EndScreens {
  private:
	Texture win;
	Texture lose;
	void DrawQuad(float sx, float sy);

  public:
	EndScreens();
	void DrawWin();
	void DrawLose();
};

#endif
