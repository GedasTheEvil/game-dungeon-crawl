// Drawing the poison drips (plan poison-drip-trap; dungeon_spikes.cpp has their rules): a dark stain on the ceiling,
// the lump of venom swelling in it, wobbling in the telegraph, then falling into the glossy green puddle on the floor
// with a splash, the venom pouring after it while the drop is out; small drops ring the puddle in between. All of it
// from the cycle and the clock (spike_cycle.h), nothing kept.
#include "dungeon.h"
#include "../state/assets.h"
#include "../graphics/fire.h"
#include "../graphics/lighting.h"
#include "../graphics/render_config.h"
#include "spike_cycle.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr float T = RenderConfig::TILE_SIZE;
constexpr float PI = 3.14159265f;
constexpr float DRIP_Z = RenderConfig::MONSTER_DEPTH; // in the middle of the corridor, where the player walks
constexpr float PUDDLE_RX = 15.f, PUDDLE_RZ = 11.f, PUDDLE_LIFT = 0.3f; // world units
constexpr float STAIN_RX = 12.f, STAIN_RZ = 10.f;
constexpr float LUMP_MIN = 1.2f, LUMP_MAX = 4.6f; // radius, right after a drop and just before the next
constexpr float LUMP_WOBBLE = 0.22f;
constexpr int LUMP_FALL_MS = 180; // from the ceiling into the puddle, at the start of the drop's time out
constexpr int SPLASH_MS = 450;
constexpr int STREAM_DROPS = 5; // the venom pouring after the lump, while the drop is out
constexpr int STREAM_FALL_MS = 220;
constexpr int SMALL_FALL_MS = 260; // a small drop from the stain to the puddle
constexpr int RIPPLE_MS = 700;
// The small drops: so long after the venom stops pouring, and the last two so long before the lump falls (in the
// telegraph). drip_plips.wav and drip_creak.wav (mechanism_sounds.py) plip as they land.
constexpr int SMALL_DROPS_AFTER_OUT_MS[] = {400, 1000, 1600};
constexpr int LAST_DROPS_BEFORE_MS[] = {560, 380};
constexpr FireStyle SPLASH = {36, 420.f, 5.f, 9.f, 3.f, 1.5f, 0.3f, &Fire::VENOM_RAMP};
constexpr Lighting::LightDef PUDDLE_GLOW = {0.45f, 0.75f, 0.15f, 38.f, 0.f};

struct Rgba {
	float r, g, b, a;
};
constexpr Rgba VENOM_DEEP = {0.12f, 0.34f, 0.04f, 0.92f};
constexpr Rgba VENOM_EDGE = {0.08f, 0.24f, 0.03f, 0.55f};
constexpr Rgba VENOM_BRIGHT = {0.4f, 0.72f, 0.14f, 0.92f};
constexpr Rgba SHEEN = {0.75f, 0.95f, 0.55f, 0.45f};
constexpr Rgba STAIN = {0.04f, 0.09f, 0.02f, 0.85f};

void colour(const Rgba& c, float fade = 1.f) { glColor4f(c.r, c.g, c.b, c.a * fade); }

// A flat ellipse at height y facing up (or down, the ceiling's), from `inner` at the middle to `outer` at the rim.
void disc(float x, float y, float z, float rx, float rz, const Rgba& inner, const Rgba& outer) {
	constexpr int SIDES = 24;
	glBegin(GL_TRIANGLE_FAN);
	colour(inner);
	glVertex3f(x, y, z);
	colour(outer);
	for (int k = 0; k <= SIDES; k++) {
		const float a = 2.f * PI * static_cast<float>(k) / SIDES;
		glVertex3f(x + rx * std::cos(a), y, z + rz * std::sin(a));
	}
	glEnd();
}

// A ripple: a thin ring on the puddle, `p` 0..1 over its life, spreading and fading.
void ripple(float x, float y, float z, float p) {
	constexpr int SIDES = 24;
	constexpr float WIDTH = 0.7f;
	const float r = 1.f + p * (PUDDLE_RX * 0.85f - 1.f), k = PUDDLE_RZ / PUDDLE_RX;
	glBegin(GL_TRIANGLE_STRIP);
	colour(SHEEN, 1.f - p);
	for (int s = 0; s <= SIDES; s++) {
		const float a = 2.f * PI * static_cast<float>(s) / SIDES;
		glVertex3f(x + r * std::cos(a), y, z + r * k * std::sin(a));
		glVertex3f(x + (r + WIDTH) * std::cos(a), y, z + (r + WIDTH) * k * std::sin(a));
	}
	glEnd();
}

// A blob of venom, radii (rx, ry) round (x, y, z), with a glint up on its left: wet, glossy.
void blob(float x, float y, float z, float rx, float ry, float fade = 1.f) {
	constexpr int SLICES = 10, STACKS = 8;
	for (int pass = 0; pass < 2; pass++) {
		const float s = pass == 0 ? 1.f : 0.32f;
		const float cx = pass == 0 ? x : x - rx * 0.35f, cy = pass == 0 ? y : y + ry * 0.35f;
		const float cz = pass == 0 ? z : z + rx * 0.55f;
		colour(pass == 0 ? VENOM_BRIGHT : SHEEN, fade);
		for (int i = 0; i < STACKS; i++) {
			const float a0 = PI * static_cast<float>(i) / STACKS - PI / 2, a1 = a0 + PI / STACKS;
			glBegin(GL_TRIANGLE_STRIP);
			for (int j = 0; j <= SLICES; j++) {
				const float b = 2 * PI * static_cast<float>(j) / SLICES;
				for (float a : {a0, a1})
					glVertex3f(
						cx + s * rx * std::cos(a) * std::cos(b),
						cy + s * ry * std::sin(a),
						cz + s * rx * std::cos(a) * std::sin(b)
					);
			}
			glEnd();
		}
	}
}

// A small drop that left the stain ageMs ago: falling, then the ripple it makes.
void smallDrop(float x, float floor, float ceiling, int ageMs) {
	if (ageMs < 0)
		return;
	if (ageMs < SMALL_FALL_MS) {
		const float p = static_cast<float>(ageMs) / SMALL_FALL_MS;
		blob(x, ceiling - (ceiling - floor) * p * p, DRIP_Z, 0.7f, 1.1f);
	} else if (ageMs < SMALL_FALL_MS + RIPPLE_MS)
		ripple(x, floor, DRIP_Z, static_cast<float>(ageMs - SMALL_FALL_MS) / RIPPLE_MS);
}
} // namespace

//======================================================================================
// Same frame as the tile loop in Draw(): cell (col0, row0) of the window sits at the origin.
void Dungeon::drawPoisonDrips(const CellRect& drawn) {
	const int now = GameClock::now();
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
	bool set = false;
	for (int j = drawn.row0; j < drawn.row0 + drawn.rows; j++)
		for (int i = drawn.col0; i < drawn.col0 + drawn.cols; i++) {
			if (!IsInBounds(i, j) || MapAt(i, j).fixture.type != PoisonDrip)
				continue;
			if (!set) {
				set = true;
				sim.assets->textures.nullTex.Bind(); // white: the colour alone
				Lighting::setEmissive(true); // venom glows a little: the trap reads in the dark
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDisable(GL_CULL_FACE);
				glDepthMask(GL_FALSE);
			}
			const Fixture& f = MapAt(i, j).fixture;
			const float x = static_cast<float>(i - view().originCol) * T + RenderConfig::TILE_HALF;
			const float y = static_cast<float>(j - view().originRow) * T;
			const float floor = y + PUDDLE_LIFT, ceiling = y + T;
			const int at = spikeCycleMs(f, now), out = spikeOutMs(f), period = spikePeriodMs(f);
			disc(x, ceiling - 0.2f, DRIP_Z, STAIN_RX, STAIN_RZ, STAIN, Rgba{STAIN.r, STAIN.g, STAIN.b, 0.f});
			disc(x, floor, DRIP_Z, PUDDLE_RX, PUDDLE_RZ, VENOM_DEEP, VENOM_EDGE);
			disc(x - 4.f, floor + 0.05f, DRIP_Z + 3.f, 5.f, 2.5f, SHEEN, Rgba{SHEEN.r, SHEEN.g, SHEEN.b, 0.f});
			for (const int after : SMALL_DROPS_AFTER_OUT_MS)
				smallDrop(x, floor, ceiling, at - out - after);
			for (const int before : LAST_DROPS_BEFORE_MS) // the last ones' ripples run on into the drop
				smallDrop(x, floor, ceiling, (at - (period - before) + period) % period);
			if (at >= out) { // swelling in the stain, wobbling in the telegraph
				const float grow = static_cast<float>(at - out) / static_cast<float>(period - out);
				const float r = LUMP_MIN + (LUMP_MAX - LUMP_MIN) * grow;
				const int toOut = period - at;
				const float tel = toOut < SPIKES_TELEGRAPH_MS ? 1.f - static_cast<float>(toOut) / SPIKES_TELEGRAPH_MS
															  : 0.f;
				const float w = LUMP_WOBBLE * tel * std::sin(static_cast<float>(at) * 0.035f * (1.f + tel));
				const float ry = r * (1.f - w) * (1.f + 0.4f * tel); // it sags as it gets ready to drop
				blob(x, ceiling - ry * 0.3f, DRIP_Z, r * 0.55f, ry * 0.45f); // the neck it hangs from
				blob(x, ceiling - ry * (0.9f + 0.3f * tel), DRIP_Z, r * (1.f + 0.5f * w), ry);
				continue;
			}
			if (at < LUMP_FALL_MS) { // the lump falls, stretched by its speed
				const float p = static_cast<float>(at) / LUMP_FALL_MS;
				blob(
					x,
					ceiling - LUMP_MAX - (ceiling - LUMP_MAX - floor) * p * p,
					DRIP_Z,
					LUMP_MAX * 0.8f,
					LUMP_MAX * 1.5f
				);
				continue;
			}
			const int landed = at - LUMP_FALL_MS;
			if (landed < SPLASH_MS) {
				Fire::draw(
					SPLASH,
					x,
					floor + 1.f,
					DRIP_Z,
					static_cast<uint32_t>(MapIndex(i, j)) * 2654435761u,
					1.f - static_cast<float>(landed) / SPLASH_MS
				);
				sim.assets->textures.nullTex.Bind(); // the fire binds its sprite and sets its own blending
				Lighting::setEmissive(true);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glDepthMask(GL_FALSE);
			}
			if (landed < RIPPLE_MS)
				ripple(x, floor + 0.05f, DRIP_Z, static_cast<float>(landed) / RIPPLE_MS);
			const float pour = 1.f - static_cast<float>(landed) / static_cast<float>(out - LUMP_FALL_MS);
			for (int k = 0; k < STREAM_DROPS; k++) { // the venom pouring after it, thinning out
				const float p = std::fmod(
					static_cast<float>(landed) / STREAM_FALL_MS + static_cast<float>(k) / STREAM_DROPS,
					1.f
				);
				const float r = 0.4f + 1.2f * pour;
				blob(x, ceiling - (ceiling - floor) * p, DRIP_Z, r, r * 1.8f, std::min(1.f, 2.f * pour));
			}
		}
	if (set)
		Lighting::setEmissive(false);
	glPopAttrib();
	glColor4f(1, 1, 1, 1);
}
//======================================================================================
// Same frame as addLights.
void Dungeon::addPoisonDripLights(const CellRect& drawn) {
	const CellRect lit = drawn.grown(1);
	for (int j = lit.row0; j < lit.row0 + lit.rows; j++)
		for (int i = lit.col0; i < lit.col0 + lit.cols; i++)
			if (IsInBounds(i, j) && MapAt(i, j).fixture.type == PoisonDrip)
				Lighting::add(
					static_cast<float>(i - view().originCol) * T + RenderConfig::TILE_HALF,
					static_cast<float>(j - view().originRow) * T + 2.f,
					DRIP_Z,
					PUDDLE_GLOW,
					static_cast<uint32_t>(MapIndex(i, j))
				);
}
