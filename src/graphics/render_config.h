#ifndef RENDER_CONFIG_H
#define RENDER_CONFIG_H

namespace RenderConfig {
constexpr float TILE_SIZE = 40.f;
constexpr float TILE_HALF = TILE_SIZE / 2.f;
constexpr float TILE_RENDER_Y = -120.f;
constexpr float PARTICLE_DRIFT = 0.1f;
constexpr float MONSTER_OFFSET_X = 40.f;
constexpr float MONSTER_OFFSET_Z = 10.f;
constexpr float MONSTER_DEPTH = MONSTER_OFFSET_Z - 30.f; // z of the monsters' and the player's bodies (Monster::Draw)
constexpr float ITEM_OFFSET_X = 20.f;
constexpr float ITEM_OFFSET_Z = 10.f;
} // namespace RenderConfig

#endif
