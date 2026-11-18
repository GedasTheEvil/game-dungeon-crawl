#include "particles_draw.h"
#include "../render/gl_includes.h"
#include "../render/lighting.h"
#include "../render/matrix_stack.h"
#include "../core/timer.h"

void Particles::Draw(const ParticleSystem& system) {
	if (!system.Live() || !shown)
		return;

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	Batch& world = Lighting::batch();
	const Rgb& colour = system.Colour();
	world.colour(colour.r, colour.g, colour.b, 0.6f);

	gMv.push();
	gMv.translate(system.X(), system.Y(), system.Z());
	world.style.line.pointSize = 8.f;
	world.begin(Primitive::Points);

	const ParticleSystem::Particle* pt = system.Points();
	for (int i = 0; i < ParticleSystem::PARTICLE_COUNT; i++)
		world.vertex(pt[i].x, pt[i].y, pt[i].z);

	world.end();
	world.style.line.pointSize = 1.f;
	gMv.pop();

	world.colour(1, 1, 1, 1);
	glDisable(GL_BLEND);
}

void Particles::DrawBleedDrops(float height, int seed) {
	if (!shown)
		return;
	constexpr int DROPS = 4;
	constexpr int FALL_MS = 900; // from the middle to the feet
	Batch& world = Lighting::batch();
	world.style.line.pointSize = 5.f;
	world.colour(0.6f, 0.03f, 0.03f, 1.f);
	world.begin(Primitive::Points);
	for (int i = 0; i < DROPS; i++) {
		const int ms = (GameClock::now() + i * FALL_MS / DROPS + seed * 97) % FALL_MS;
		const float t = static_cast<float>(ms) / FALL_MS;
		const float side = static_cast<float>((i * 37 + seed) % 7 - 3) * 0.05f;
		world.vertex(side * height, height * 0.55f * (1.f - t * t), 0.2f * height);
	}
	world.end();
	world.style.line.pointSize = 1.f;
	world.colour(1, 1, 1, 1);
}
