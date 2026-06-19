#include "particles.h"
#include <cmath>
#include <GL/gl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "render_config.h"

#define CDefaultSystemLife 100
#define CDefaultParticleLife 100

ParSys::ParSys() {
	life = CDefaultSystemLife; // default lifetime
	for (int i = 0; i < CMaxPart; i++) {
		pt[i].x = 0;
		pt[i].y = 0;
		pt[i].z = 0;
		pt[i].life = CDefaultParticleLife;
	}

	// Default blood color (red)
	colour.r = 0.7f;
	colour.g = 0.1f;
	colour.b = 0.1f;

	frameTimer = std::make_unique<timer>(5);
	decayTimer = std::make_unique<timer>(5);
}

ParSys::ParSys(int life) {
	this->life = life;
	for (int i = 0; i < CMaxPart; i++) {
		pt[i].x = 0;
		pt[i].y = 0;
		pt[i].z = 0;
		pt[i].life = CDefaultParticleLife;
	}

	// Default blood color (red)
	colour.r = 0.7f;
	colour.g = 0.1f;
	colour.b = 0.1f;

	frameTimer = std::make_unique<timer>(5);
	decayTimer = std::make_unique<timer>(5);
}

ParSys::~ParSys() { life = 0; }

void ParSys::Fall() {
	if (!decayTimer->TimePassed())
		return;

	if (life <= 0)
		return;

	for (int i = 0; i < CMaxPart; i++) {
		pt[i].x += RenderConfig::PARTICLE_DRIFT * static_cast<float>(sin(i));
		pt[i].y -= RenderConfig::PARTICLE_DRIFT;
		pt[i].z += RenderConfig::PARTICLE_DRIFT * static_cast<float>(cos(i));
		pt[i].life--;
		if (pt[i].life <= 0) {
			pt[i].x = 0;
			pt[i].y = 0;
			pt[i].z = 0;
			pt[i].life = CDefaultParticleLife;
		}
	}

	life--;
}

void ParSys::Explode() {
	if (!frameTimer->TimePassed())
		return;

	if (life <= 0)
		return;

	for (int i = 0; i < CMaxPart; i++) {
		// Chaotic explosion: completely random directions and forces
		float explosionForce = ((rand() % 100) / 100.0f) * 0.6f + 0.05f; // 0.05-0.65 force
		float randomAngle = (rand() % 360) * 3.14159f / 180.0f;			 // Completely random angle

		// Add multiple layers of randomness for chaotic explosion
		float chaosX = ((rand() % 100 - 50) / 100.0f) * 0.3f; // ±0.3 chaos
		float chaosY = ((rand() % 100 - 50) / 100.0f) * 0.3f; // ±0.3 chaos
		float upwardBurst = ((rand() % 40) / 100.0f) * 0.25f; // Random upward burst

		pt[i].x += explosionForce * cos(randomAngle) * ((rand() % 4) + 1) + chaosX;
		pt[i].y += explosionForce * sin(randomAngle) * ((rand() % 4) + 1) + upwardBurst + chaosY;
		pt[i].z += explosionForce * ((rand() % 100 - 50) / 100.0f) * 0.5f; // Random depth

		pt[i].life--;
		if (pt[i].life <= 0) {
			pt[i].x = 0;
			pt[i].y = 0;
			pt[i].z = 0;
			pt[i].life = CDefaultParticleLife;
		}
	}

	life--;
}

void ParSys::Draw() {

	glPointSize(8);

	if (life <= 0)
		return;

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glColor4f(colour.r, colour.g, colour.b, 0.6);

	glPushMatrix();
	glTranslatef(x, y, z);
	glBegin(GL_POINTS);

	for (int i = 0; i < CMaxPart; i++)
		glVertex3f(pt[i].x, pt[i].y, pt[i].z);

	glEnd();
	glPopMatrix();

	glColor4f(1, 1, 1, 1);
	glDisable(GL_BLEND);
}

void ParSys::setCords(float x, float y, float z) {
	this->x = x;
	this->y = y;
	this->z = z;
}

void ParSys::setBloodColor(float r, float g, float b) {
	colour.r = r;
	colour.g = g;
	colour.b = b;
}

void ParSys::Reset() {
	life = CDefaultSystemLife; // default lifetime
}
