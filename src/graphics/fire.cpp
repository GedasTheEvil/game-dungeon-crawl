#include "fire.h"

#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include "lighting.h"
#include "../core/timer.h"
#include "render_config.h"

namespace {
constexpr int MAX_PARTICLES = 128;
constexpr int SPRITE_SIZE = 32;
constexpr float SMOKE_FROM = 0.55f; // life fraction where a particle counts as smoke (drawn in the first pass)

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

void colourAt(const FireRamp& ramp, float p, float smoke, Particle& out) {
	int k = 1;
	while (k < ramp.count - 1 && ramp.keys[k].p < p)
		k++;
	const FireKey& lo = ramp.keys[k - 1];
	const FireKey& hi = ramp.keys[k];
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
		colourAt(*style.ramp, p, style.smoke, q);
	}
	return n;
}
} // namespace

namespace {
constexpr FireKey FLAME_KEYS[] = {
	{0.00f, 1.00f, 0.92f, 0.55f, 0.00f, 1.f}, // born white-yellow, fades in
	{0.06f, 1.00f, 0.85f, 0.35f, 0.90f, 1.f}, // yellow
	{0.25f, 1.00f, 0.52f, 0.12f, 0.85f, 1.f}, // orange
	{0.45f, 0.85f, 0.18f, 0.05f, 0.70f, 0.8f}, // red
	{0.62f, 0.28f, 0.14f, 0.10f, 0.50f, 0.3f}, // embers turning to smoke
	{0.80f, 0.16f, 0.15f, 0.15f, 0.35f, 0.f}, // dark grey
	{1.00f, 0.12f, 0.12f, 0.12f, 0.00f, 0.f}, // gone
};
constexpr FireKey VENOM_KEYS[] = {
	{0.00f, 0.60f, 0.88f, 0.30f, 0.00f, 1.f}, // born light green, fades in
	{0.06f, 0.55f, 0.84f, 0.24f, 0.80f, 1.f}, // light tint of VENOM_GREEN
	{0.25f, 0.45f, 0.75f, 0.15f, 0.85f, 1.f}, // VENOM_GREEN
	{0.45f, 0.25f, 0.50f, 0.08f, 0.70f, 0.8f}, // darker green
	{0.62f, 0.15f, 0.28f, 0.08f, 0.50f, 0.3f}, // dying green
	{0.80f, 0.18f, 0.22f, 0.15f, 0.35f, 0.f}, // sickly grey-green smoke
	{1.00f, 0.13f, 0.15f, 0.12f, 0.00f, 0.f}, // gone
};
} // namespace
const FireRamp Fire::FLAME_RAMP = {FLAME_KEYS, sizeof(FLAME_KEYS) / sizeof(FLAME_KEYS[0])};
const FireRamp Fire::VENOM_RAMP = {VENOM_KEYS, sizeof(VENOM_KEYS) / sizeof(VENOM_KEYS[0])};

void Fire::draw(const FireStyle& style, float x, float y, float z, uint32_t seed, float fade) {
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
			glColor4f(q.r * fade, q.g * fade, q.b * fade, q.a * fade); // premultiplied: all four
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

// Rows of flames up the box, one to a few across: the bigger the body, the more.
void Fire::body(float x, float y, float z, float halfWidth, float height, uint32_t seed) {
	constexpr float ROW_STEP = 7.f; // world units between the rows, about
	constexpr float ACROSS_STEP = 4.f;
	const int rows = std::clamp(static_cast<int>(std::lround(height / ROW_STEP)), 1, 4);
	const int across = std::clamp(static_cast<int>(std::lround(halfWidth / ACROSS_STEP)), 1, 3);
	const float step = height / static_cast<float>(rows);
	for (int r = 0; r < rows; r++)
		for (int i = 0; i < across; i++) {
			const float fx = x + halfWidth * (2.f * (static_cast<float>(i) + 0.5f) / static_cast<float>(across) - 1.f);
			draw(BODY, fx, y + step * static_cast<float>(r), z, seed + static_cast<uint32_t>(r * 4 + i) * 7919u);
		}
}

void Dust::draw(float x, float y, float z, float progress, uint32_t seed) {
	constexpr int GRAINS = 48;
	constexpr float FALL_MS = 700.f; // one grain from the ceiling to the floor
	constexpr float DROP = 40.f; // ceiling to floor, world units
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

void Grit::burst(float x, float y, float z, int ageMs, bool fromCeiling, uint32_t seed) {
	constexpr int GRAINS = 64;
	constexpr int CLOUDS = 10; // soft puffs of dust round the spot
	constexpr float SPRAY_S = 0.5f; // grains leave the spot over this time, while the minion comes out
	constexpr float GRAVITY = 260.f;
	const float t = static_cast<float>(ageMs) / 1000.f;
	const float end = static_cast<float>(BURST_MS) / 1000.f;
	if (t < 0.f || t >= end)
		return;
	ensureSprite();

	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	float rl = std::sqrt(m[0] * m[0] + m[4] * m[4] + m[8] * m[8]);
	float ul = std::sqrt(m[1] * m[1] + m[5] * m[5] + m[9] * m[9]);
	float rx = m[0] / rl, ry = m[4] / rl, rz = m[8] / rl;
	float ux = m[1] / ul, uy = m[5] / ul, uz = m[9] / ul;
	auto quad = [&](float cx, float cy, float cz, float size, float r, float g, float b, float a) {
		float h = size / 2.f;
		glColor4f(r * a, g * a, b * a, a); // premultiplied
		glTexCoord2f(0, 0);
		glVertex3f(cx - (rx + ux) * h, cy - (ry + uy) * h, cz - (rz + uz) * h);
		glTexCoord2f(1, 0);
		glVertex3f(cx + (rx - ux) * h, cy + (ry - uy) * h, cz + (rz - uz) * h);
		glTexCoord2f(1, 1);
		glVertex3f(cx + (rx + ux) * h, cy + (ry + uy) * h, cz + (rz + uz) * h);
		glTexCoord2f(0, 1);
		glVertex3f(cx - (rx - ux) * h, cy - (ry - uy) * h, cz - (rz - uz) * h);
	};
	const float fade = 1.f - t / end;
	// Pale sand thrown up from the floor; dark grit and old dust from the ceiling, to show against the lit walls.
	const float r = fromCeiling ? 0.30f : 0.78f, g = fromCeiling ? 0.25f : 0.66f, b = fromCeiling ? 0.19f : 0.46f;

	Lighting::setEmissive(true);
	glBindTexture(GL_TEXTURE_2D, gSprite);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glBegin(GL_QUADS);
	for (int i = 0; i < CLOUDS; i++) { // spreads and thins out
		uint32_t h = mix(seed ^ mix(static_cast<uint32_t>(i) + 0x68e31da4U));
		float grow = 1.f - (1.f - t / end) * (1.f - t / end);
		float cx = x + (unit01(h) - 0.5f) * (8.f + 22.f * grow);
		float cy = y + (fromCeiling ? -1.f : 1.f) * (1.5f + 5.f * unit01(h >> 16) * grow);
		float cz = z + (unit01(mix(h)) - 0.5f) * 10.f;
		quad(cx, cy, cz, 7.f + 9.f * grow, r, g, b, 0.35f * fade);
	}
	for (int i = 0; i < GRAINS; i++) {
		uint32_t h = mix(seed ^ mix(static_cast<uint32_t>(i) + 0x85ebca6bU));
		float age = t - SPRAY_S * unit01(h);
		if (age <= 0.f)
			continue;
		uint32_t h2 = mix(h);
		float vx = (unit01(h2) - 0.5f) * 60.f;
		float vz = (unit01(h2 >> 16) - 0.5f) * 30.f;
		float vy = fromCeiling ? -20.f * unit01(h >> 16) : 45.f + 55.f * unit01(h >> 16);
		float cx = x + vx * age * (fromCeiling ? 0.3f : 1.f);
		float cy = y + vy * age - GRAVITY * age * age / 2.f;
		float cz = z + vz * age;
		if (!fromCeiling)
			cy = std::fmax(cy, y); // settles on the floor
		else if (cy < y - RenderConfig::TILE_SIZE)
			continue; // reached the floor
		float size = 1.2f + 2.f * unit01(mix(h2));
		quad(cx, cy, cz, size, r, g, b, 0.9f * fade);
	}
	glEnd();
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glColor4f(1, 1, 1, 1);
	Lighting::setEmissive(false);
}

namespace {
// Camera-facing sprite quads in the current modelview, premultiplied, emissive, without depth writes: begin, add,
// end.
struct Sprites {
	float rx = 1.f, ry = 0.f, rz = 0.f, ux = 0.f, uy = 1.f, uz = 0.f;
	void begin() {
		ensureSprite();
		float m[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, m);
		const float rl = std::sqrt(m[0] * m[0] + m[4] * m[4] + m[8] * m[8]);
		const float ul = std::sqrt(m[1] * m[1] + m[5] * m[5] + m[9] * m[9]);
		rx = m[0] / rl, ry = m[4] / rl, rz = m[8] / rl;
		ux = m[1] / ul, uy = m[5] / ul, uz = m[9] / ul;
		Lighting::setEmissive(true);
		glBindTexture(GL_TEXTURE_2D, gSprite);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		glBegin(GL_QUADS);
	}
	void add(float cx, float cy, float cz, float size, float r, float g, float b, float a) const {
		const float h = size / 2.f;
		glColor4f(r * a, g * a, b * a, a);
		glTexCoord2f(0, 0);
		glVertex3f(cx - (rx + ux) * h, cy - (ry + uy) * h, cz - (rz + uz) * h);
		glTexCoord2f(1, 0);
		glVertex3f(cx + (rx - ux) * h, cy + (ry - uy) * h, cz + (rz - uz) * h);
		glTexCoord2f(1, 1);
		glVertex3f(cx + (rx + ux) * h, cy + (ry + uy) * h, cz + (rz + uz) * h);
		glTexCoord2f(0, 1);
		glVertex3f(cx - (rx - ux) * h, cy - (ry - uy) * h, cz - (rz - uz) * h);
	}
	static void end() {
		glEnd();
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glColor4f(1, 1, 1, 1);
		Lighting::setEmissive(false);
	}
};
constexpr float SAND_R = 0.80f, SAND_G = 0.66f, SAND_B = 0.44f; // pale desert sand
} // namespace

void Grit::puff(float x, float y, float z, int ageMs, int lifeMs, uint32_t seed) {
	constexpr int CLOUDS = 5;
	constexpr int GRAINS = 10;
	const float t = static_cast<float>(ageMs) / static_cast<float>(lifeMs);
	if (t < 0.f || t >= 1.f)
		return;
	Sprites s;
	s.begin();
	const float fade = 1.f - t;
	const float grow = 1.f - fade * fade;
	for (int i = 0; i < CLOUDS; i++) {
		const uint32_t h = mix(seed ^ mix(static_cast<uint32_t>(i) + 0x3c6ef372U));
		const float cx = x + (unit01(h) - 0.5f) * (6.f + 14.f * grow);
		const float cy = y + 1.f + 4.f * unit01(h >> 16) * grow;
		const float cz = z + (unit01(mix(h)) - 0.5f) * 8.f;
		s.add(cx, cy, cz, 4.f + 6.f * grow, SAND_R, SAND_G, SAND_B, 0.28f * fade);
	}
	const float sec = t * static_cast<float>(lifeMs) / 1000.f;
	for (int i = 0; i < GRAINS; i++) {
		const uint32_t h = mix(seed ^ mix(static_cast<uint32_t>(i) + 0x9e3779b9U));
		const float vx = (unit01(h) - 0.5f) * 40.f;
		const float vy = 20.f + 30.f * unit01(h >> 16);
		const float cy = std::fmax(y, y + vy * sec - 130.f * sec * sec);
		s.add(x + vx * sec, cy, z + (unit01(mix(h)) - 0.5f) * 8.f, 1.f + unit01(mix(h) >> 16), SAND_R, SAND_G, SAND_B,
			  0.8f * fade);
	}
	Sprites::end();
}

void Grit::pour(float x, float y, float z, int ageMs, int lifeMs, uint32_t seed) {
	constexpr int GRAINS = 140;
	constexpr int CLOUDS = 8;
	constexpr float FALL_S = 0.35f; // a grain's way from the ceiling to the floor
	const float t = static_cast<float>(ageMs) / 1000.f;
	const float life = static_cast<float>(lifeMs) / 1000.f;
	if (t < 0.f || t >= life + FALL_S)
		return;
	Sprites s;
	s.begin();
	const float drop = RenderConfig::TILE_SIZE;
	for (int i = 0; i < GRAINS; i++) { // grains leave the crack all through the pour, thinner at its end
		const uint32_t h = mix(seed ^ mix(static_cast<uint32_t>(i) + 0x85ebca6bU));
		const float leave = life * unit01(h);
		const float age = t - leave;
		if (age < 0.f || age >= FALL_S)
			continue;
		const float p = age / FALL_S;
		const float spread = 3.f + 9.f * p; // the stream fans out as it falls
		const float cx = x + (unit01(mix(h)) - 0.5f) * spread;
		const float cy = y - drop * p * p;
		const float cz = z + (unit01(mix(h) >> 16) - 0.5f) * spread * 0.6f;
		const float thin = 1.f - 0.6f * leave / life;
		s.add(cx, cy, cz, 1.4f + 1.6f * unit01(h >> 16), SAND_R, SAND_G, SAND_B, 0.9f * thin);
	}
	for (int i = 0; i < CLOUDS; i++) { // dust where it lands
		const uint32_t h = mix(seed ^ mix(static_cast<uint32_t>(i) + 0x68e31da4U));
		const float k = std::fmin(t / (life + FALL_S), 1.f);
		const float cx = x + (unit01(h) - 0.5f) * (10.f + 20.f * k);
		const float cy = y - drop + 2.f + 6.f * unit01(h >> 16) * k;
		const float cz = z + (unit01(mix(h)) - 0.5f) * 12.f;
		s.add(cx, cy, cz, 8.f + 8.f * k, SAND_R, SAND_G, SAND_B, 0.3f * (1.f - k));
	}
	Sprites::end();
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

void SunBeam::shaft(float x, float y, float z, uint32_t seed) {
	constexpr float R = 1.f, G = 0.86f, B = 0.6f;
	constexpr float HEIGHT = RenderConfig::TILE_SIZE;
	constexpr int MOTES = 14;
	constexpr float MOTE_MS = 6000.f;
	Sprites s;
	s.begin();
	glEnd();
	glBlendFunc(GL_ONE, GL_ONE);
	glBegin(GL_QUADS);
	struct Layer {
		float top, bottom, bright; // half widths at the ceiling and the floor
	};
	constexpr Layer LAYERS[] = {{7.f, 13.f, 0.16f}, {2.5f, 5.f, 0.3f}};
	for (const Layer& l : LAYERS) {
		glColor4f(R * l.bright * 0.5f, G * l.bright * 0.5f, B * l.bright * 0.5f, 1);
		glTexCoord2f(0, 0.5f);
		glVertex3f(x - l.bottom * s.rx, y, z - l.bottom * s.rz);
		glTexCoord2f(1, 0.5f);
		glVertex3f(x + l.bottom * s.rx, y, z + l.bottom * s.rz);
		glColor4f(R * l.bright, G * l.bright, B * l.bright, 1);
		glTexCoord2f(1, 0.5f);
		glVertex3f(x + l.top * s.rx, y + HEIGHT, z + l.top * s.rz);
		glTexCoord2f(0, 0.5f);
		glVertex3f(x - l.top * s.rx, y + HEIGHT, z - l.top * s.rz);
	}
	const float pool = 0.22f; // on the floor
	glColor4f(R * pool, G * pool, B * pool, 1);
	glTexCoord2f(0, 0);
	glVertex3f(x - 16.f, y + 0.3f, z - 8.f);
	glTexCoord2f(0, 1);
	glVertex3f(x - 16.f, y + 0.3f, z + 8.f);
	glTexCoord2f(1, 1);
	glVertex3f(x + 16.f, y + 0.3f, z + 8.f);
	glTexCoord2f(1, 0);
	glVertex3f(x + 16.f, y + 0.3f, z - 8.f);
	glEnd();
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	const float now = static_cast<float>(GameClock::now());
	for (int i = 0; i < MOTES; i++) { // dust sinking slowly through the light
		const uint32_t h = mix(seed ^ mix(static_cast<uint32_t>(i) + 0x27d4eb2fU));
		const float p = std::fmod(now / MOTE_MS + unit01(h), 1.f);
		const float my = y + HEIGHT * (1.f - p);
		const float mx =
			x + (unit01(h >> 16) - 0.5f) * 8.f * (0.5f + p) + 1.5f * std::sin(now * 0.001f + static_cast<float>(i));
		s.add(mx, my, z + (unit01(mix(h)) - 0.5f) * 6.f, 1.2f, R, G, B, 0.5f * std::sin(3.1416f * p));
	}
	Sprites::end();
}

void SunBeam::ray(float x0, float y0, float x1, float y1, float z, uint32_t seed) {
	constexpr float R = 1.f, G = 0.87f, B = 0.6f;
	constexpr float MOTES_PER_TILE = 6.f;
	constexpr float MOTE_MS = 5000.f;
	const bool vertical = std::fabs(x1 - x0) < std::fabs(y1 - y0);
	const float length = std::hypot(x1 - x0, y1 - y0);
	if (length <= 0.f)
		return;
	Sprites s;
	s.begin();
	glEnd();
	glBlendFunc(GL_ONE, GL_ONE);
	glBegin(GL_QUADS);
	// Across the beam: the camera's right for a column (it turns to face the camera), up for a row.
	const float wx = vertical ? s.rx : 0.f, wy = vertical ? 0.f : 1.f, wz = vertical ? s.rz : 0.f;
	struct Layer {
		float halfWidth, bright;
	};
	constexpr Layer LAYERS[] = {{8.f, 0.22f}, {3.f, 0.5f}, {1.f, 0.6f}};
	for (const Layer& l : LAYERS) {
		const float hx = wx * l.halfWidth, hy = wy * l.halfWidth, hz = wz * l.halfWidth;
		glColor4f(R * l.bright, G * l.bright, B * l.bright, 1);
		glTexCoord2f(0, 0.5f);
		glVertex3f(x0 - hx, y0 - hy, z - hz);
		glTexCoord2f(1, 0.5f);
		glVertex3f(x0 + hx, y0 + hy, z + hz);
		glTexCoord2f(1, 0.5f);
		glVertex3f(x1 + hx, y1 + hy, z + hz);
		glTexCoord2f(0, 0.5f);
		glVertex3f(x1 - hx, y1 - hy, z - hz);
	}
	glEnd();
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	const float now = static_cast<float>(GameClock::now());
	const int motes = static_cast<int>(MOTES_PER_TILE * length / RenderConfig::TILE_SIZE) + 1;
	for (int i = 0; i < motes; i++) { // dust drifting slowly along the light, glittering in it
		const uint32_t h = mix(seed ^ mix(static_cast<uint32_t>(i) + 0x5bd1e995U));
		const float p = std::fmod(now / MOTE_MS + unit01(h), 1.f);
		const float off = (unit01(h >> 16) - 0.5f) * 5.f + 1.2f * std::sin(now * 0.0013f + static_cast<float>(i));
		const float mx = x0 + (x1 - x0) * p + wx * off, my = y0 + (y1 - y0) * p + wy * off;
		s.add(mx, my, z + wz * off + (unit01(mix(h)) - 0.5f) * 5.f, 1.1f, R, G, B, 0.6f * std::sin(3.1416f * p));
	}
	Sprites::end();
}

void SunBeam::glow(float x, float y, float z, float size, float strength) {
	constexpr float R = 1.f, G = 0.88f, B = 0.62f;
	Sprites s;
	s.begin();
	glEnd();
	glBlendFunc(GL_ONE, GL_ONE);
	glBegin(GL_QUADS);
	s.add(x, y, z, size, R * strength, G * strength, B * strength, 1.f);
	s.add(x, y, z, size * 0.4f, strength, strength, strength * 0.9f, 1.f);
	Sprites::end();
}
