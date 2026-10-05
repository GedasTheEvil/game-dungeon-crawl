#include "particles.h"
#include <cmath>
#include <GL/gl.h>
#include <cstdlib>
#include "render_config.h"

void ParticleSystem::Fall() {
	if (!decayTimer.TimePassed())
		return;

	if (life <= 0)
		return;

	for (int i = 0; i < PARTICLE_COUNT; i++) {
		pt[i].x += RenderConfig::PARTICLE_DRIFT * static_cast<float>(sin(i));
		pt[i].y -= RenderConfig::PARTICLE_DRIFT;
		pt[i].z += RenderConfig::PARTICLE_DRIFT * static_cast<float>(cos(i));
		pt[i].life--;
		if (pt[i].life <= 0) {
			pt[i].x = 0;
			pt[i].y = 0;
			pt[i].z = 0;
			pt[i].life = DEFAULT_LIFE;
		}
	}

	life--;
}

void ParticleSystem::Explode() {
	if (!frameTimer.TimePassed())
		return;

	if (life <= 0)
		return;

	for (int i = 0; i < PARTICLE_COUNT; i++) {
		// Chaotic explosion: completely random directions and forces
		float explosionForce = (static_cast<float>(rng.below(100)) / 100.0f) * 0.6f + 0.05f; // 0.05-0.65 force
		float randomAngle = static_cast<float>(rng.below(360)) * 3.14159f / 180.0f;			 // Completely random angle

		// Add multiple layers of randomness for chaotic explosion
		float chaosX = (static_cast<float>(rng.below(100) - 50) / 100.0f) * 0.3f; // ±0.3 chaos
		float chaosY = (static_cast<float>(rng.below(100) - 50) / 100.0f) * 0.3f; // ±0.3 chaos
		float upwardBurst = (static_cast<float>(rng.below(40)) / 100.0f) * 0.25f; // Random upward burst

		pt[i].x += explosionForce * std::cos(randomAngle) * static_cast<float>(rng.below(4) + 1) + chaosX;
		pt[i].y += explosionForce * std::sin(randomAngle) * static_cast<float>(rng.below(4) + 1) + upwardBurst + chaosY;
		pt[i].z += explosionForce * (static_cast<float>(rng.below(100) - 50) / 100.0f) * 0.5f; // Random depth

		pt[i].life--;
		if (pt[i].life <= 0) {
			pt[i].x = 0;
			pt[i].y = 0;
			pt[i].z = 0;
			pt[i].life = DEFAULT_LIFE;
		}
	}

	life--;
}

void ParticleSystem::Draw() {

	glPointSize(8);

	if (life <= 0 || !shown)
		return;

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glColor4f(colour.r, colour.g, colour.b, 0.6);

	glPushMatrix();
	glTranslatef(x, y, z);
	glBegin(GL_POINTS);

	for (int i = 0; i < PARTICLE_COUNT; i++)
		glVertex3f(pt[i].x, pt[i].y, pt[i].z);

	glEnd();
	glPopMatrix();

	glColor4f(1, 1, 1, 1);
	glDisable(GL_BLEND);
}

void ParticleSystem::setCords(float x, float y, float z) {
	this->x = x;
	this->y = y;
	this->z = z;
}

void ParticleSystem::setBloodColor(float r, float g, float b) {
	colour.r = r;
	colour.g = g;
	colour.b = b;
}

void ParticleSystem::Splash(int extent) {
	setCords(static_cast<float>(rng.below(extent)), static_cast<float>(rng.below(extent)), 0);
	Reset();
}

void ParticleSystem::Reset() {
	life = DEFAULT_LIFE; // default lifetime
}

void ParticleSystem::Stop() { life = 0; }
