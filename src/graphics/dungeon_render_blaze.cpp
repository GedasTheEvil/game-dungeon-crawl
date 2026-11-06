// A floor set burning by Babi's big coal (plan babi-boss, the rules in dungeon_blaze.cpp): flames over the floor of
// the cell, flaring up as it catches and dying down at the end, and their light.
#include "dungeon.h"
#include "../graphics/fire.h"
#include "../graphics/lighting.h"
#include "../graphics/render_config.h"
#include <algorithm>

namespace {
constexpr float T = RenderConfig::TILE_SIZE;
constexpr int CATCH_MS = 250; // the flames flare up from nothing
constexpr Lighting::LightDef BLAZE_LIGHT = {1.f, 0.5f, 0.18f, 70.f, 0.6f};
// Where the fires stand on the floor of the cell: along the row, in depth (of a tile, from the back wall).
constexpr float SPOTS[][2] = {{0.2f, 0.35f}, {0.5f, 0.6f}, {0.8f, 0.35f}, {0.35f, 0.8f}, {0.68f, 0.85f}};

// 0..1: how high the flames of a blaze from startMs to endMs burn now.
float strength(int startMs, int endMs, int now) {
	const float in = std::min(1.f, static_cast<float>(now - startMs) / CATCH_MS);
	const float out = std::min(1.f, static_cast<float>(endMs - now) / BLAZE_FADE_MS);
	return std::clamp(std::min(in, out), 0.f, 1.f);
}
} // namespace

//======================================================================================
// Same window and frame as drawSummonEffects.
void Dungeon::drawBlazes() {
	const int now = GameClock::now();
	const auto col0 = static_cast<float>(view().originCol), row0 = static_cast<float>(view().originRow);
	for (const Blaze& b : blazes) {
		const int col = b.cell % MAP_WIDTH, row = b.cell / MAP_WIDTH;
		if (!drawnCells.contains(static_cast<float>(col) + 0.5f, row))
			continue;
		const float fade = strength(b.startMs, b.endMs, now);
		for (size_t k = 0; k < std::size(SPOTS); k++)
			Fire::draw(
				Fire::FLOOR_BLAZE,
				(static_cast<float>(col) + SPOTS[k][0] - col0) * T,
				(static_cast<float>(row) - row0) * T,
				-SPOTS[k][1] * T,
				static_cast<uint32_t>(b.cell * 31 + static_cast<int>(k) * 977 + b.startMs),
				fade
			);
	}
}
//======================================================================================
// Same frame as addLights.
void Dungeon::addBlazeLights() {
	const int now = GameClock::now();
	const auto col0 = static_cast<float>(view().originCol), row0 = static_cast<float>(view().originRow);
	for (const Blaze& b : blazes) {
		const int col = b.cell % MAP_WIDTH, row = b.cell / MAP_WIDTH;
		Lighting::LightDef light = BLAZE_LIGHT;
		light.radius *= strength(b.startMs, b.endMs, now);
		Lighting::add(
			(static_cast<float>(col) + 0.5f - col0) * T,
			(static_cast<float>(row) - row0) * T + 0.3f * T,
			-0.5f * T,
			light,
			static_cast<uint32_t>(b.cell)
		);
	}
}
