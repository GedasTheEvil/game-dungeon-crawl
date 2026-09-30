#include "hud.h"
#include <GL/gl.h>

namespace Hud {
void drawBar(float left, float bottom, float width, float height, float ratio, float red, float green, float blue) {
	glColor3f(1, 1, 1);
	glBegin(GL_LINE_LOOP);
	glVertex3f(left, bottom, 0);
	glVertex3f(left + width, bottom, 0);
	glVertex3f(left + width, bottom + height, 0);
	glVertex3f(left, bottom + height, 0);
	glEnd();

	glColor3f(red, green, blue);
	glBegin(GL_QUADS);
	glVertex3f(left, bottom, 0);
	glVertex3f(left + width * ratio, bottom, 0);
	glVertex3f(left + width * ratio, bottom + height, 0);
	glVertex3f(left, bottom + height, 0);
	glEnd();
}
} // namespace Hud
