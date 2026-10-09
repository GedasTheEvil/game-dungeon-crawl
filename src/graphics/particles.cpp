#include "particles.h"
#include <GL/gl.h>

void Particles::Draw(const ParticleSystem& system) {

	glPointSize(8);

	if (!system.Live() || !shown)
		return;

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	const Rgb& colour = system.Colour();
	glColor4f(colour.r, colour.g, colour.b, 0.6);

	glPushMatrix();
	glTranslatef(system.X(), system.Y(), system.Z());
	glBegin(GL_POINTS);

	const ParticleSystem::Particle* pt = system.Points();
	for (int i = 0; i < ParticleSystem::PARTICLE_COUNT; i++)
		glVertex3f(pt[i].x, pt[i].y, pt[i].z);

	glEnd();
	glPopMatrix();

	glColor4f(1, 1, 1, 1);
	glDisable(GL_BLEND);
}
