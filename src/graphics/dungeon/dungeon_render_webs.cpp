// The Mother of Webs' webs over the floor (plan camel-spider-boss, the rules in dungeon_webs.cpp): pale threads from
// the middle of the cell out to its edges and rings round it, a faint sheet between them. A cut web fades, a burning
// one goes up in flames.
#include "dungeon_view.h"
#include "world_draw.h"
#include "../assets.h"
#include "../fire.h"
#include "../../render/lighting.h"
#include "../../sim/model/render_config.h"
#include "../../level/tuning/gameplay_monster_kinds.h"
#include "../../render/gl/gl_includes.h"
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

// Same window and frame as drawSummonEffects.
void DungeonView::drawWebs() {
	if (d.hazards.webs.empty())
		return;
	const int now = GameClock::now();
	const auto col0 = static_cast<float>(d.view().originCol), row0 = static_cast<float>(d.view().originRow);
	assets.textures.nullTex.Bind();
	Lighting::setEmissive(true); // seen in the dark too, a faint grey
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	for (const Hazards::Web& w : d.hazards.webs) {
		const int col = w.cell % Dungeon::MAP_WIDTH, row = w.cell / Dungeon::MAP_WIDTH;
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
		world().begin(Primitive::TriangleFan); // the sheet
		world().colour(0.6f * burn, 0.59f * burn, 0.55f * burn, 0.06f * fade);
		world().vertex(cx, y, cz);
		for (int s = 0; s <= SPOKES; s++)
			world().vertex(cx + ex[s % SPOKES], y, cz + ez[s % SPOKES]);
		world().end();
		world().style.line.width = 1.5f;
		world().colour(0.62f * burn, 0.61f * burn, 0.57f * burn, 0.32f * fade);
		world().begin(Primitive::Lines);
		for (int s = 0; s < SPOKES; s++) {
			world().vertex(cx, y, cz);
			world().vertex(cx + ex[s], y, cz + ez[s]);
		}
		for (int k = 1; k <= RINGS; k++) {
			const float r = static_cast<float>(k) / (RINGS + 0.4f);
			for (int s = 0; s < SPOKES; s++) {
				const int n = (s + 1) % SPOKES;
				const float mx = (ex[s] + ex[n]) * 0.5f * SAG, mz = (ez[s] + ez[n]) * 0.5f * SAG;
				world().vertex(cx + ex[s] * r, y, cz + ez[s] * r);
				world().vertex(cx + mx * r, y, cz + mz * r);
				world().vertex(cx + mx * r, y, cz + mz * r);
				world().vertex(cx + ex[n] * r, y, cz + ez[n] * r);
			}
		}
		world().end();
		world().style.line = {};
	}
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	Lighting::setEmissive(false);
	world().colour(1, 1, 1);
	for (const Hazards::Web& w : d.hazards.webs) // after the threads: the sprites blend over them
		if (w.burnt && w.goneMs >= 0) {
			const int col = w.cell % Dungeon::MAP_WIDTH, row = w.cell / Dungeon::MAP_WIDTH;
			const float fade = 1.f - static_cast<float>(now - w.goneMs) / static_cast<float>(WEB_BURN_MS);
			for (int k = -1; k <= 1; k++)
				Fire::draw(
					Fire::BODY,
					(static_cast<float>(col) + 0.5f + 0.3f * static_cast<float>(k) - col0) * T,
					(static_cast<float>(row) - row0) * T,
					-0.5f * T,
					static_cast<uint32_t>(w.cell * 3 + k + 1),
					std::max(fade, 0.f)
				);
		}
}
