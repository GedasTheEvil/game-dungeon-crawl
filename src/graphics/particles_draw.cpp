#include "particles_draw.h"
#include "../render/gl_includes.h"
#include "../core/timer.h"

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

void Particles::DrawBleedDrops(float height, int seed) {
	if (!shown)
		return;
	constexpr int DROPS = 4;
	constexpr int FALL_MS = 900; // from the middle to the feet
	glPointSize(5);
	glColor4f(0.6f, 0.03f, 0.03f, 1.f);
	glBegin(GL_POINTS);
	for (int i = 0; i < DROPS; i++) {
		const int ms = (GameClock::now() + i * FALL_MS / DROPS + seed * 97) % FALL_MS;
		const float t = static_cast<float>(ms) / FALL_MS;
		const float side = static_cast<float>((i * 37 + seed) % 7 - 3) * 0.05f;
		glVertex3f(side * height, height * 0.55f * (1.f - t * t), 0.2f * height);
	}
	glEnd();
	glColor4f(1, 1, 1, 1);
}
