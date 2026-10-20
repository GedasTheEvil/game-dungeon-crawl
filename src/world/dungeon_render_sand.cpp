// Sand on the floor (plan desert-chapter): the drifts, the quicksand's swirl, the dust of the steps, the sand worm's
// ripple and the sand falls pouring down.
#include "dungeon.h"
#include "../state/assets.h"
#include "../entities/player.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include "../graphics/render_config.h"
#include "../graphics/fire.h"
#include "../graphics/lighting.h"

namespace {
constexpr float T = RenderConfig::TILE_SIZE;
constexpr int DRIFT_NX = 10, DRIFT_NZ = 5; // the drift's grid over a cell
constexpr float DRIFT_HEIGHT = 0.11f * T; // its crest over the floor
constexpr float DRIFT_EDGE = 0.35f; // of a cell: the drift runs out towards a neighbour without sand
constexpr float RIPPLE = 0.12f * DRIFT_HEIGHT;
constexpr float QUICKSAND_DIP = 0.55f; // of the height: its middle sags
constexpr float SWIRL_NEAR = 2.5f; // tiles: the quicksand swirls for a player this near
constexpr float SWIRL_TWIST = 0.9f; // radians at the middle, to and fro
constexpr int SAND_POUR_MS = ROCK_FALL_MS + 600; // a sand fall's stream, from the end of its warning
constexpr float POUR_DEPTH = 0.5f; // of the cell's depth: in the middle of the corridor, as the rock's
constexpr float SHAFT_DEPTH = 0.45f; // of the cell's depth: the crack runs along the middle of the ceiling

float smooth(float e0, float e1, float x) {
	const float k = std::clamp((x - e0) / (e1 - e0), 0.f, 1.f);
	return k * k * (3.f - 2.f * k);
}
} // namespace

//======================================================================================
// The drift's height (world units) at (u, v) of cell (i, j): u 0..1 left to right, v 0..1 back wall to front. Ripples
// run in level coordinates, so drifts side by side meet; towards an open neighbour without sand the drift runs out,
// against rock it piles up, and it piles against the back wall too. Quicksand sags in the middle.
float Dungeon::driftHeight(int i, int j, float u, float v) const {
	const float x = static_cast<float>(i) + u;
	float h = DRIFT_HEIGHT * (0.8f + 0.35f * (1.f - v) * (1.f - v));
	h += RIPPLE * std::sin(6.2832f * (2.3f * x + 0.4f * v)) + 0.5f * RIPPLE * std::sin(6.2832f * (5.1f * x - 0.9f * v));
	for (int side : {-1, 1}) {
		const float edge = side < 0 ? u : 1.f - u; // 0 at that side's border
		if (isRock(i + side, j))
			h += 0.5f * DRIFT_HEIGHT * (1.f - smooth(0.f, 0.5f, edge));
		else if (!IsInBounds(i + side, j) || !inSand(MapAt(i + side, j)))
			h *= smooth(0.f, DRIFT_EDGE, edge);
	}
	if (inQuicksand(MapAt(i, j))) {
		const float r = std::hypot(u - 0.5f, (v - 0.5f) * 0.8f) / 0.45f;
		h *= 1.f - QUICKSAND_DIP * std::max(0.f, 1.f - r) * std::max(0.f, 1.f - r);
	}
	return std::max(h, 0.f);
}
//======================================================================================
// The drift as a grid over the cell's floor, its front edge closed by a strip of sand down to the floor. Quicksand is
// darker, and up close its sand twists slowly round the middle (the tell).
void Dungeon::drawSandDrift(int i, int j) {
	const bool quick = inQuicksand(MapAt(i, j));
	float twist = 0.f;
	if (quick) {
		const float near = std::hypot(mapX - static_cast<float>(i) - 0.5f, (mapY - static_cast<float>(j)) * 2.f);
		const float k = 1.f - smooth(1.f, SWIRL_NEAR, near);
		twist = k * SWIRL_TWIST * std::sin(static_cast<float>(GameClock::now()) * 0.0011f + static_cast<float>(i));
	}
	auto texAt = [&](float u, float v, float out[2]) {
		const float du = u - 0.5f, dv = v - 0.5f;
		const float a = twist * std::max(0.f, 1.f - std::hypot(du, dv) / 0.5f);
		out[0] = 0.5f + du * std::cos(a) - dv * std::sin(a);
		out[1] = 0.5f + du * std::sin(a) + dv * std::cos(a);
	};
	float hs[DRIFT_NX + 1][DRIFT_NZ + 1];
	for (int a = 0; a <= DRIFT_NX; a++)
		for (int b = 0; b <= DRIFT_NZ; b++)
			hs[a][b] = driftHeight(i, j, static_cast<float>(a) / DRIFT_NX, static_cast<float>(b) / DRIFT_NZ);

	sim.assets->decor.floorTex[FLOOR_SAND].Bind();
	if (quick)
		glColor3f(0.78f, 0.68f, 0.55f);
	constexpr float DX = T / DRIFT_NX, DZ = T / DRIFT_NZ;
	for (int b = 0; b < DRIFT_NZ; b++) {
		glBegin(GL_TRIANGLE_STRIP);
		for (int a = 0; a <= DRIFT_NX; a++)
			for (int k : {b + 1, b}) { // front row first: the strip faces up
				const int a0 = std::max(a - 1, 0), a1 = std::min(a + 1, DRIFT_NX);
				const int k0 = std::max(k - 1, 0), k1 = std::min(k + 1, DRIFT_NZ);
				const float sx = (hs[a1][k] - hs[a0][k]) / (DX * static_cast<float>(a1 - a0));
				const float sz = (hs[a][k1] - hs[a][k0]) / (DZ * static_cast<float>(k1 - k0)); // v grows to the front
				const float n = std::sqrt(sx * sx + sz * sz + 1.f);
				glNormal3f(-sx / n, 1.f / n, -sz / n);
				float st[2];
				texAt(static_cast<float>(a) / DRIFT_NX, static_cast<float>(k) / DRIFT_NZ, st);
				glTexCoord2f(st[0], 1.f - st[1]);
				glVertex3f(static_cast<float>(a) * DX, hs[a][k], -T + static_cast<float>(k) * DZ);
			}
		glEnd();
	}
	glBegin(GL_QUAD_STRIP); // the front edge
	glNormal3f(0, 0, 1);
	for (int a = 0; a <= DRIFT_NX; a++) {
		const float s = static_cast<float>(a) / DRIFT_NX;
		glTexCoord2f(s, 0.f);
		glVertex3f(static_cast<float>(a) * DX, 0.f, 0.f);
		glTexCoord2f(s, hs[a][DRIFT_NZ] / T);
		glVertex3f(static_cast<float>(a) * DX, hs[a][DRIFT_NZ], 0.f);
	}
	glEnd();
	glColor3f(1, 1, 1);
}
//======================================================================================
// Same window and frame as drawSummonEffects: the dust of the steps in sand, a sand worm's ripple under the sand, the
// sand falls pouring down.
void Dungeon::drawSandEffects() {
	const int now = GameClock::now();
	const auto col0 = static_cast<float>(view().originCol), row0 = static_cast<float>(view().originRow);
	puffs.erase(
		std::remove_if(puffs.begin(), puffs.end(), [now](const Hole& h) { return now - h.startMs >= SAND_PUFF_MS; }),
		puffs.end());
	for (const Hole& p : puffs)
		Grit::puff((p.x - col0) * T, (static_cast<float>(p.row) - row0) * T + DRIFT_HEIGHT, RenderConfig::MONSTER_DEPTH,
				   now - p.startMs, SAND_PUFF_MS, static_cast<uint32_t>(p.startMs));

	for (const Monster& mon : monsters) {
		const std::optional<float> x = mon.Active() && mon.Type()->sandBurrow ? mon.BurrowRipple() : std::nullopt;
		if (!x)
			continue;
		const float cx = (*x - col0) * T, cy = (static_cast<float>(mon.Row()) - row0) * T + DRIFT_HEIGHT;
		drawRipple(cx, cy, RenderConfig::MONSTER_DEPTH, now);
	}

	pours.erase(std::remove_if(pours.begin(), pours.end(),
							   [now](const Hole& h) { return now - h.startMs >= SAND_POUR_MS + 400; }),
				pours.end());
	for (const Motion& m : fallingRocks) // the stream starts as the warning ends
		if (isSandFall(map[m.cell]) && now - m.startMs >= ROCK_WARN_MS &&
			std::none_of(pours.begin(), pours.end(),
						 [&](const Hole& h) { return h.startMs == m.startMs + ROCK_WARN_MS; }))
			pours.push_back(
				{static_cast<float>(m.cell % MAP_WIDTH) + 0.5f, m.cell / MAP_WIDTH, m.startMs + ROCK_WARN_MS});
	for (int j = drawnCells.row0; j < drawnCells.row0 + drawnCells.rows; j++) // after the opaque scene
		for (int i = drawnCells.col0; i < drawnCells.col0 + drawnCells.cols; i++)
			if (lightShaft(i, j))
				SunBeam::shaft((static_cast<float>(i) + 0.5f - col0) * T, (static_cast<float>(j) - row0) * T,
							   -T * SHAFT_DEPTH, static_cast<uint32_t>(MapIndex(i, j)));
	for (const Hole& p : pours)
		Grit::pour((p.x - col0) * T, (static_cast<float>(p.row + 1) - row0) * T, -T * (1.f - POUR_DEPTH),
				   now - p.startMs, SAND_POUR_MS, static_cast<uint32_t>(p.startMs));
}
//======================================================================================
bool Dungeon::lightShaft(int i, int j) const {
	return IsInBounds(i, j) && !isRock(i, j) && isRock(i, j + 1) &&
		   decoration.surface[MapIndex(i, j)].ceiling == CEILING_SANDSTONE_CRACKED;
}
//======================================================================================
// A worm under the sand: a low mound of sand that heaves, dark grains rolling off it.
void Dungeon::drawRipple(float x, float y, float z, int now) const {
	constexpr float RX = 13.f, RZ = 8.f, H = 6.f; // world units
	constexpr int PUFF_MS = 900;
	constexpr int SIDES = 16;
	const float heave = 0.7f + 0.3f * std::sin(static_cast<float>(now) * 0.012f);
	sim.assets->decor.floorTex[FLOOR_SAND].Bind();
	glColor3f(0.8f, 0.68f, 0.52f); // stirred-up sand, a shade darker
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0, 1, 0);
	glTexCoord2f(0.5f, 0.5f);
	glVertex3f(x, y + H * heave, z);
	for (int k = 0; k <= SIDES; k++) {
		const float a = 6.2832f * static_cast<float>(k) / SIDES;
		glNormal3f(std::cos(a) * 0.5f, 0.85f, std::sin(a) * 0.5f);
		glTexCoord2f(0.5f + 0.5f * std::cos(a), 0.5f + 0.5f * std::sin(a));
		glVertex3f(x + RX * std::cos(a), y - 0.5f, z + RZ * std::sin(a));
	}
	glEnd();
	glColor3f(1, 1, 1);
	// Sand trickling off the heaving mound, puff after puff.
	Grit::puff(x, y + H * heave, z, now % PUFF_MS, PUFF_MS, static_cast<uint32_t>(now / PUFF_MS));
}
