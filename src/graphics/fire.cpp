#include "fire.h"

#include <GL/gl.h>
#include <cmath>
#include "lighting.h"
#include "../core/timer.h"

namespace {
constexpr int MAX_PARTICLES = 128;
constexpr int SPRITE_SIZE = 32;
constexpr float SMOKE_FROM = 0.55f; // life fraction where a particle counts as smoke (drawn in the first pass)

// Colour over a particle's life. add = 1 blends additively (glowing flame), 0 blends over (smoke that darkens).
struct Key {
	float p, r, g, b, a, add;
};
constexpr Key RAMP[] = {
	{0.00f, 1.00f, 0.92f, 0.55f, 0.00f, 1.f},  // born white-yellow, fades in
	{0.06f, 1.00f, 0.85f, 0.35f, 0.90f, 1.f},  // yellow
	{0.25f, 1.00f, 0.52f, 0.12f, 0.85f, 1.f},  // orange
	{0.45f, 0.85f, 0.18f, 0.05f, 0.70f, 0.8f}, // red
	{0.62f, 0.28f, 0.14f, 0.10f, 0.50f, 0.3f}, // embers turning to smoke
	{0.80f, 0.16f, 0.15f, 0.15f, 0.35f, 0.f},  // dark grey
	{1.00f, 0.12f, 0.12f, 0.12f, 0.00f, 0.f},  // gone
};
constexpr int RAMP_KEYS = sizeof(RAMP) / sizeof(RAMP[0]);

struct Particle {
	float x, y, z, size, p;
	float r, g, b, a; // premultiplied colour, alpha = a * (1 - add)
};

GLuint gSprite = 0;

uint32_t mix(uint32_t h) { // lowbias32 integer hash
	h ^= h >> 16;
	h *= 0x7feb352dU;
	h ^= h >> 15;
	h *= 0x846ca68bU;
	h ^= h >> 16;
	return h;
}

float unit01(uint32_t h) { return static_cast<float>(h & 0xffffU) / 65535.f; }

// Soft round sprite, premultiplied (rgb = alpha = falloff) so the blend works for flame and smoke alike.
void ensureSprite() {
	if (gSprite != 0)
		return;
	unsigned char px[SPRITE_SIZE * SPRITE_SIZE * 4];
	for (int y = 0; y < SPRITE_SIZE; y++)
		for (int x = 0; x < SPRITE_SIZE; x++) {
			float dx = (static_cast<float>(x) + 0.5f) / SPRITE_SIZE * 2.f - 1.f;
			float dy = (static_cast<float>(y) + 0.5f) / SPRITE_SIZE * 2.f - 1.f;
			float k = std::fmax(0.f, 1.f - (dx * dx + dy * dy));
			auto v = static_cast<unsigned char>(255.f * k * k);
			unsigned char* o = &px[static_cast<size_t>(y * SPRITE_SIZE + x) * 4];
			o[0] = o[1] = o[2] = o[3] = v;
		}
	glGenTextures(1, &gSprite);
	glBindTexture(GL_TEXTURE_2D, gSprite);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SPRITE_SIZE, SPRITE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
}

void colourAt(float p, float smoke, Particle& out) {
	int k = 1;
	while (k < RAMP_KEYS - 1 && RAMP[k].p < p)
		k++;
	const Key& lo = RAMP[k - 1];
	const Key& hi = RAMP[k];
	float f = std::fmin(1.f, std::fmax(0.f, (p - lo.p) / (hi.p - lo.p)));
	float add = lo.add + (hi.add - lo.add) * f;
	float a = (lo.a + (hi.a - lo.a) * f) * (smoke + (1.f - smoke) * add);
	out.r = (lo.r + (hi.r - lo.r) * f) * a;
	out.g = (lo.g + (hi.g - lo.g) * f) * a;
	out.b = (lo.b + (hi.b - lo.b) * f) * a;
	out.a = a * (1.f - add);
}

int simulate(const FireStyle& style, uint32_t seed, Particle* out) {
	float t = static_cast<float>(GameClock::now());
	int n = style.count < MAX_PARTICLES ? style.count : MAX_PARTICLES;
	for (int i = 0; i < n; i++) {
		// Life and phase are fixed per particle, so its age advances smoothly; the rest re-rolls every cycle.
		uint32_t base = mix(seed ^ mix(static_cast<uint32_t>(i) + 0x9e3779b9U));
		float life = style.lifeMs * (0.7f + 0.6f * unit01(base));
		float u = t / life + unit01(base >> 16);
		float cycle = std::floor(u);
		float p = u - cycle;
		uint32_t h = mix(base ^ static_cast<uint32_t>(cycle));

		float ang = unit01(h) * 6.2831853f;
		float rad = style.spawnRadius * std::sqrt(unit01(h >> 16));
		h = mix(h);
		float converge = 1.f - 0.6f * std::fmin(1.f, p / 0.45f); // flame tongues narrow as they rise
		float drift = style.sway * p * p;
		float swayPhase = unit01(h) * 6.2831853f;

		Particle& q = out[i];
		q.p = p;
		q.x = std::cos(ang) * rad * converge + drift * std::sin(t * 0.0021f + swayPhase);
		q.y = style.height * p * (0.7f + 0.3f * p);
		q.z = std::sin(ang) * rad * converge * 0.6f + drift * 0.5f * std::cos(t * 0.0017f + swayPhase);
		float grow = p < 0.5f ? 1.f - 0.5f * p : 0.75f + 1.6f * (p - 0.5f); // flame shrinks, smoke spreads
		q.size = style.size * grow * (0.75f + 0.5f * unit01(h >> 16));
		colourAt(p, style.smoke, q);
	}
	return n;
}
} // namespace

void Fire::draw(const FireStyle& style, float x, float y, float z, uint32_t seed) {
	Particle parts[MAX_PARTICLES];
	int n = simulate(style, seed, parts);
	ensureSprite();

	// Billboard axes: the modelview's first two rows are the camera's right and up in local space.
	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	float rl = std::sqrt(m[0] * m[0] + m[4] * m[4] + m[8] * m[8]);
	float ul = std::sqrt(m[1] * m[1] + m[5] * m[5] + m[9] * m[9]);
	float rx = m[0] / rl, ry = m[4] / rl, rz = m[8] / rl;
	float ux = m[1] / ul, uy = m[5] / ul, uz = m[9] / ul;

	Lighting::setEmissive(true);
	glBindTexture(GL_TEXTURE_2D, gSprite);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glBegin(GL_QUADS);
	for (int pass = 0; pass < 2; pass++) // smoke first, so the flames glow on top of it
		for (int i = 0; i < n; i++) {
			const Particle& q = parts[i];
			if ((q.p >= SMOKE_FROM) != (pass == 0))
				continue;
			float h = q.size / 2.f;
			float cx = x + q.x, cy = y + q.y, cz = z + q.z;
			glColor4f(q.r, q.g, q.b, q.a);
			glTexCoord2f(0, 0);
			glVertex3f(cx - (rx + ux) * h, cy - (ry + uy) * h, cz - (rz + uz) * h);
			glTexCoord2f(1, 0);
			glVertex3f(cx + (rx - ux) * h, cy + (ry - uy) * h, cz + (rz - uz) * h);
			glTexCoord2f(1, 1);
			glVertex3f(cx + (rx + ux) * h, cy + (ry + uy) * h, cz + (rz + uz) * h);
			glTexCoord2f(0, 1);
			glVertex3f(cx - (rx - ux) * h, cy - (ry - uy) * h, cz - (rz - uz) * h);
		}
	glEnd();
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glColor4f(1, 1, 1, 1);
	Lighting::setEmissive(false);
}

void Dust::draw(float x, float y, float z, float progress, uint32_t seed) {
	constexpr int GRAINS = 48;
	constexpr float FALL_MS = 700.f; // one grain from the ceiling to the floor
	constexpr float DROP = 40.f;	 // ceiling to floor, world units
	ensureSprite();

	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	float rl = std::sqrt(m[0] * m[0] + m[4] * m[4] + m[8] * m[8]);
	float ul = std::sqrt(m[1] * m[1] + m[5] * m[5] + m[9] * m[9]);
	float rx = m[0] / rl, ry = m[4] / rl, rz = m[8] / rl;
	float ux = m[1] / ul, uy = m[5] / ul, uz = m[9] / ul;

	float t = static_cast<float>(GameClock::now());
	int alive = static_cast<int>(static_cast<float>(GRAINS) * std::fmin(1.f, 0.25f + progress));

	Lighting::setEmissive(true);
	glBindTexture(GL_TEXTURE_2D, gSprite);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glBegin(GL_QUADS);
	for (int i = 0; i < alive; i++) {
		uint32_t base = mix(seed ^ mix(static_cast<uint32_t>(i) + 0x85ebca6bU));
		float u = t / FALL_MS + unit01(base);
		float p = u - std::floor(u);
		uint32_t h = mix(base ^ static_cast<uint32_t>(std::floor(u)));
		float cx = x + (unit01(h) - 0.5f) * 22.f;
		float cy = y - DROP * p * p; // accelerates
		float cz = z + (unit01(h >> 16) - 0.5f) * 14.f;
		float size = 0.8f + 1.6f * unit01(mix(h));
		float a = 0.55f * (1.f - p);
		float h2 = size / 2.f;
		glColor4f(0.55f * a, 0.47f * a, 0.36f * a, a); // sandstone grit, premultiplied
		glTexCoord2f(0, 0);
		glVertex3f(cx - (rx + ux) * h2, cy - (ry + uy) * h2, cz - (rz + uz) * h2);
		glTexCoord2f(1, 0);
		glVertex3f(cx + (rx - ux) * h2, cy + (ry - uy) * h2, cz + (rz - uz) * h2);
		glTexCoord2f(1, 1);
		glVertex3f(cx + (rx + ux) * h2, cy + (ry + uy) * h2, cz + (rz + uz) * h2);
		glTexCoord2f(0, 1);
		glVertex3f(cx - (rx - ux) * h2, cy - (ry - uy) * h2, cz - (rz - uz) * h2);
	}
	glEnd();
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glColor4f(1, 1, 1, 1);
	Lighting::setEmissive(false);
}

float SunBeam::strength(float progress) {
	if (progress <= 0.f || progress >= 1.f)
		return 0.f;
	if (progress < 0.15f) // opens fast
		return progress / 0.15f;
	if (progress < 0.5f)
		return 1.f;
	float f = (progress - 0.5f) / 0.5f; // then fades slowly
	return 1.f - f * f * (3.f - 2.f * f);
}

void SunBeam::draw(float x, float y, float z, float progress) {
	constexpr float HEIGHT = 160.f; // well above the top of the screen, the ceiling hides the rest
	constexpr int MOTES = 40;
	constexpr float RISE_MS = 1100.f;
	constexpr float RISE = 45.f;
	constexpr float R = 1.f, G = 0.83f, B = 0.45f; // warm sunlight
	float s = strength(progress);
	if (s <= 0.f)
		return;
	ensureSprite();

	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	float rl = std::sqrt(m[0] * m[0] + m[4] * m[4] + m[8] * m[8]);
	float ul = std::sqrt(m[1] * m[1] + m[5] * m[5] + m[9] * m[9]);
	float rx = m[0] / rl, ry = m[4] / rl, rz = m[8] / rl;
	float ux = m[1] / ul, uy = m[5] / ul, uz = m[9] / ul;
	// The beam stays upright: it turns only round the vertical axis to face the camera.
	float hl = std::sqrt(rx * rx + rz * rz);
	float bx = hl > 0.f ? rx / hl : 1.f, bz = hl > 0.f ? rz / hl : 0.f;

	Lighting::setEmissive(true);
	glBindTexture(GL_TEXTURE_2D, gSprite);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDepthMask(GL_FALSE);
	glBegin(GL_QUADS);

	// Shaft: a wide soft glow and a narrow core. The sprite's middle row is the soft side-to-side falloff.
	float narrow = progress < 0.5f ? 1.f : 1.f - 0.7f * (progress - 0.5f) / 0.5f; // closes as it fades
	struct Layer {
		float halfWidth, bright;
	};
	constexpr Layer LAYERS[] = {{15.f, 0.35f}, {5.5f, 0.75f}};
	constexpr float ROW_Y[] = {0.f, 30.f, HEIGHT};
	constexpr float ROW_K[] = {0.75f, 1.f, 0.3f}; // brightest round the chest, dims upwards
	for (const Layer& l : LAYERS)
		for (int r = 0; r < 2; r++) {
			float w0 = l.halfWidth * narrow * (1.f - 0.25f * ROW_Y[r] / HEIGHT);
			float w1 = l.halfWidth * narrow * (1.f - 0.25f * ROW_Y[r + 1] / HEIGHT);
			float k0 = l.bright * ROW_K[r] * s, k1 = l.bright * ROW_K[r + 1] * s;
			glColor4f(R * k0, G * k0, B * k0, 1);
			glTexCoord2f(0, 0.5f);
			glVertex3f(x - bx * w0, y + ROW_Y[r], z - bz * w0);
			glTexCoord2f(1, 0.5f);
			glVertex3f(x + bx * w0, y + ROW_Y[r], z + bz * w0);
			glColor4f(R * k1, G * k1, B * k1, 1);
			glTexCoord2f(1, 0.5f);
			glVertex3f(x + bx * w1, y + ROW_Y[r + 1], z + bz * w1);
			glTexCoord2f(0, 0.5f);
			glVertex3f(x - bx * w1, y + ROW_Y[r + 1], z - bz * w1);
		}

	// Pool of light on the floor.
	float pool = 0.6f * s;
	float px = 18.f * narrow, pz = 9.f * narrow;
	glColor4f(R * pool, G * pool, B * pool, 1);
	glTexCoord2f(0, 0);
	glVertex3f(x - px, y + 0.3f, z - pz);
	glTexCoord2f(0, 1);
	glVertex3f(x - px, y + 0.3f, z + pz);
	glTexCoord2f(1, 1);
	glVertex3f(x + px, y + 0.3f, z + pz);
	glTexCoord2f(1, 0);
	glVertex3f(x + px, y + 0.3f, z - pz);

	// Gold motes rising through the shaft.
	float t = static_cast<float>(GameClock::now());
	for (int i = 0; i < MOTES; i++) {
		uint32_t base = mix(static_cast<uint32_t>(i) * 0x27d4eb2dU + 0x165667b1U);
		float u = t / RISE_MS + unit01(base);
		float p = u - std::floor(u);
		uint32_t h = mix(base ^ static_cast<uint32_t>(std::floor(u)));
		float cx = x + (unit01(h) - 0.5f) * 14.f * narrow + 1.2f * std::sin(t * 0.004f + static_cast<float>(i));
		float cy = y + RISE * p;
		float cz = z + (unit01(h >> 16) - 0.5f) * 8.f;
		float size = 0.7f + 1.1f * unit01(mix(h));
		float a = s * std::fmin(1.f, p * 5.f) * (1.f - p); // fade in, then out towards the top
		float h2 = size / 2.f;
		glColor4f(R * a, G * a, B * 0.8f * a, 1);
		glTexCoord2f(0, 0);
		glVertex3f(cx - (rx + ux) * h2, cy - (ry + uy) * h2, cz - (rz + uz) * h2);
		glTexCoord2f(1, 0);
		glVertex3f(cx + (rx - ux) * h2, cy + (ry - uy) * h2, cz + (rz - uz) * h2);
		glTexCoord2f(1, 1);
		glVertex3f(cx + (rx + ux) * h2, cy + (ry + uy) * h2, cz + (rz + uz) * h2);
		glTexCoord2f(0, 1);
		glVertex3f(cx - (rx - ux) * h2, cy - (ry - uy) * h2, cz - (rz - uz) * h2);
	}
	glEnd();
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glColor4f(1, 1, 1, 1);
	Lighting::setEmissive(false);
}
