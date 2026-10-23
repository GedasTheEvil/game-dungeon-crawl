// The Mother of Webs' webs over the floor (plan camel-spider-boss, the rules in dungeon_webs.cpp): pale threads from
// the middle of the cell out to its edges and rings round it, a faint sheet between them. A cut web fades, a burning
// one goes up in flames.
#include "dungeon.h"
#include "../state/assets.h"
#include "../graphics/fire.h"
#include "../graphics/lighting.h"
#include "../graphics/render_config.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr float T = RenderConfig::TILE_SIZE;
constexpr int SPOKES = 9;
constexpr int RINGS = 4;
constexpr float LIFT = 0.6f; // world units over the floor, clear of its depth
constexpr float RX = 0.5f * T, RZ = 0.46f * T; // to the cell's edges along the row and in depth
constexpr float SAG = 0.82f; // a ring's thread between two spokes sags in towards the middle
} // namespace

//======================================================================================
// Same window and frame as drawSummonEffects.
void Dungeon::drawWebs() {
	if (webs.empty())
		return;
	const int now = GameClock::now();
	const auto col0 = static_cast<float>(view().originCol), row0 = static_cast<float>(view().originRow);
	sim.assets->textures.nullTex.Bind();
	Lighting::setEmissive(true); // seen in the dark too, a faint grey
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	for (const Web& w : webs) {
		const int col = w.cell % MAP_WIDTH, row = w.cell / MAP_WIDTH;
		if (!drawnCells.contains(static_cast<float>(col) + 0.5f, row))
			continue;
		const int age = w.goneMs < 0 ? 0 : now - w.goneMs;
		const int life = w.burnt ? WEB_BURN_MS : WEB_CUT_MS;
		const float fade = std::clamp(1.f - static_cast<float>(age) / static_cast<float>(life), 0.f, 1.f) *
						   std::min(1.f, static_cast<float>(now - w.laidMs) / 300.f); // spun out in a moment
		const float cx = (static_cast<float>(col) + 0.5f - col0) * T;
		const float y = (static_cast<float>(row) - row0) * T + LIFT;
		const float cz = -0.5f * T;
		// The ends of the spokes, a little uneven, the same for each web in its cell.
		float ex[SPOKES], ez[SPOKES];
		for (int s = 0; s < SPOKES; s++) {
			const float a = 2.f * static_cast<float>(M_PI) *
							(static_cast<float>(s) + 0.3f * std::sin(static_cast<float>(w.cell * 7 + s * 13))) / SPOKES;
			const float r = 0.88f + 0.12f * std::sin(static_cast<float>(w.cell + s * 5));
			ex[s] = RX * r * std::cos(a);
			ez[s] = RZ * r * std::sin(a);
		}
		const float burn = w.burnt ? 0.45f : 1.f; // the threads blacken as they burn
		glBegin(GL_TRIANGLE_FAN); // the sheet
		glColor4f(0.6f * burn, 0.59f * burn, 0.55f * burn, 0.06f * fade);
		glVertex3f(cx, y, cz);
		for (int s = 0; s <= SPOKES; s++)
			glVertex3f(cx + ex[s % SPOKES], y, cz + ez[s % SPOKES]);
		glEnd();
		glLineWidth(1.5f);
		glColor4f(0.62f * burn, 0.61f * burn, 0.57f * burn, 0.32f * fade);
		glBegin(GL_LINES);
		for (int s = 0; s < SPOKES; s++) {
			glVertex3f(cx, y, cz);
			glVertex3f(cx + ex[s], y, cz + ez[s]);
		}
		for (int k = 1; k <= RINGS; k++) {
			const float r = static_cast<float>(k) / (RINGS + 0.4f);
			for (int s = 0; s < SPOKES; s++) {
				const int n = (s + 1) % SPOKES;
				const float mx = (ex[s] + ex[n]) * 0.5f * SAG, mz = (ez[s] + ez[n]) * 0.5f * SAG;
				glVertex3f(cx + ex[s] * r, y, cz + ez[s] * r);
				glVertex3f(cx + mx * r, y, cz + mz * r);
				glVertex3f(cx + mx * r, y, cz + mz * r);
				glVertex3f(cx + ex[n] * r, y, cz + ez[n] * r);
			}
		}
		glEnd();
		glLineWidth(1.f);
	}
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	Lighting::setEmissive(false);
	glColor3f(1, 1, 1);
	for (const Web& w : webs) // after the threads: the sprites blend over them
		if (w.burnt && w.goneMs >= 0) {
			const int col = w.cell % MAP_WIDTH, row = w.cell / MAP_WIDTH;
			const float fade = 1.f - static_cast<float>(now - w.goneMs) / static_cast<float>(WEB_BURN_MS);
			for (int k = -1; k <= 1; k++)
				Fire::draw(Fire::BODY, (static_cast<float>(col) + 0.5f + 0.3f * static_cast<float>(k) - col0) * T,
						   (static_cast<float>(row) - row0) * T, -0.5f * T, static_cast<uint32_t>(w.cell * 3 + k + 1),
						   std::max(fade, 0.f));
		}
}
