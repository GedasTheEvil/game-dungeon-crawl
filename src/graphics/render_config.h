#ifndef RENDER_CONFIG_H
#define RENDER_CONFIG_H

namespace RenderConfig {
constexpr float TILE_SIZE = 40.f;
constexpr float TILE_HALF = TILE_SIZE / 2.f;
// Half water lies in a basin: its floor WATER_BASIN_DEPTH below the row's floor, the surface at WATER_SURFACE (just
// under the dry floor's edge, so a stone lip shows). Whatever stands in it is drawn sunk into the basin
// (Dungeon::WaterSink), the water up to the archaeologist's thighs.
constexpr float WATER_BASIN_DEPTH = 15.f;
constexpr float WATER_SURFACE = -3.f;
constexpr float WATER_DEPTH = WATER_BASIN_DEPTH + WATER_SURFACE; // from the basin floor to the surface
// Deep water meeting an open cell below or beside it sits on / against a rock slab this thick (Dungeon::deepWaterBox).
constexpr float WATER_SLAB = 4.f;
constexpr float WATER_SINK_RAMP = 0.3f; // tiles from a dry edge: wading in, the sink grows over this
constexpr float TILE_RENDER_Y = -120.f;
constexpr float PARTICLE_DRIFT = 0.1f;
constexpr float MONSTER_OFFSET_X = 40.f;
constexpr float MONSTER_OFFSET_Z = 10.f;
constexpr float MONSTER_DEPTH = MONSTER_OFFSET_Z - 30.f; // z of the monsters' and the player's bodies (Monster::Draw)
// z of the ladders' rails (tools/blender/models/ladder.py: 0.04 tile in front of the back wall); a climber grips there.
constexpr float LADDER_RAIL_Z = -0.96f * TILE_SIZE;
constexpr float ITEM_OFFSET_X = 20.f;
constexpr float ITEM_OFFSET_Z = 10.f;
} // namespace RenderConfig

#endif
