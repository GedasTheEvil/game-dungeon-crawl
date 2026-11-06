#ifndef TERRAIN_H
#define TERRAIN_H

// Terrain affinity (plan terrain-affinity): a monster kind's native and hostile terrains (MonsterKind::native,
// hostile) and what the terrains of the cell it stands in make of its speed, bite rate and damage (TERRAIN_ROWS).
// The cell tests are the dungeon's (Dungeon::TerrainsAt); here only the sets and the rules that combine them.

#include "gameplay_config.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>

enum class Terrain : unsigned char {
	Water, // half water (inHalfWater)
	Sand, // sand and quicksand alike (inSand)
	Darkness, // every cell that is not light
	Light, // a lit cell (Dungeon::Lit) or a sunbeam (Dungeon::Sunlit)
	Web, // a floor web (Dungeon::WebAt)
	Fire, // a real flame's own cell: a brazier, an oil lamp, a burning fire vent, a blaze; not a torch's or the sun's
};
constexpr int TERRAIN_COUNT = 6;
constexpr const char* TERRAIN_NAMES[TERRAIN_COUNT] = {"water", "sand", "darkness", "light", "web", "fire"};
static_assert(std::size(TERRAIN_ROWS) == TERRAIN_COUNT, "TERRAIN_ROWS: one row per Terrain");

// A set of terrains, one bit each. A cell can be in several (a brazier's cell is fire and light).
class Terrains {
  public:
	constexpr Terrains() = default;
	template <typename... T> constexpr explicit Terrains(Terrain first, T... rest) : bits(bit(first)) {
		((bits |= bit(rest)), ...);
	}
	[[nodiscard]] constexpr bool has(Terrain t) const { return (bits & bit(t)) != 0; }
	[[nodiscard]] constexpr bool any() const { return bits != 0; }
	constexpr void add(Terrain t) { bits |= bit(t); }
	constexpr void remove(Terrain t) { bits &= static_cast<std::uint8_t>(~bit(t)); }
	[[nodiscard]] constexpr Terrains operator&(Terrains o) const { return fromBits(bits & o.bits); }
	[[nodiscard]] constexpr Terrains operator|(Terrains o) const { return fromBits(bits | o.bits); }
	[[nodiscard]] constexpr bool operator==(const Terrains&) const = default;

  private:
	std::uint8_t bits = 0;
	[[nodiscard]] static constexpr std::uint8_t bit(Terrain t) {
		return static_cast<std::uint8_t>(1U << static_cast<unsigned>(t));
	}
	[[nodiscard]] static constexpr Terrains fromBits(unsigned b) {
		Terrains s;
		s.bits = static_cast<std::uint8_t>(b);
		return s;
	}
};

// How a cell suits a monster: native (it has a native terrain there and no hostile one), hostile (the reverse), else
// neutral (none, or native and hostile at once: they cancel out). The avoidance prefers native, then neutral.
enum class TerrainMood : unsigned char { Hostile, Neutral, Native };

[[nodiscard]] constexpr TerrainMood terrainMood(Terrains here, Terrains native, Terrains hostile) {
	const bool home = (here & native).any();
	const bool foe = (here & hostile).any();
	return home == foe ? TerrainMood::Neutral : home ? TerrainMood::Native : TerrainMood::Hostile;
}

// Its factors in a cell: native in several terrains, per factor the strongest bonus (fastest, shortest interval,
// hardest); hostile in several, the strongest penalty. No stacking. Neutral: all 1.
[[nodiscard]] constexpr TerrainFactors terrainFactors(Terrains here, Terrains native, Terrains hostile) {
	const TerrainMood mood = terrainMood(here, native, hostile);
	if (mood == TerrainMood::Neutral)
		return {};
	const Terrains mine = here & (mood == TerrainMood::Native ? native : hostile);
	const bool bonus = mood == TerrainMood::Native;
	TerrainFactors f;
	bool first = true;
	for (int t = 0; t < TERRAIN_COUNT; t++) {
		if (!mine.has(static_cast<Terrain>(t)))
			continue;
		const TerrainRow& row = TERRAIN_ROWS[static_cast<std::size_t>(t)];
		const TerrainFactors& r = bonus ? row.native : row.hostile;
		if (first) {
			f = r;
			first = false;
		} else if (bonus) {
			f = {std::max(f.speed, r.speed), std::min(f.interval, r.interval), std::max(f.damage, r.damage)};
		} else {
			f = {std::min(f.speed, r.speed), std::max(f.interval, r.interval), std::min(f.damage, r.damage)};
		}
	}
	return f;
}

// A slowed wader's (Wading::Slowed) speed factor in half water with the terrain's: a penalty does not stack with the
// wade (the lower of the two), a bonus does.
[[nodiscard]] constexpr float wadeWithTerrain(float wade, float terrain) {
	return terrain >= 1.f ? wade * terrain : std::min(wade, terrain);
}

// A hit of base damage times a terrain's damage factor, at least 1 if base was.
[[nodiscard]] inline int terrainDamage(int base, float factor) {
	if (base <= 0)
		return base;
	return std::max(1, static_cast<int>(std::lround(static_cast<float>(base) * factor)));
}

// The checker's threat factor for factors f (terrainFactors): a native's gain in speed and damage per second, weighed
// by TERRAIN_THREAT_WEIGHT. A hostile terrain's penalty is left out (1): the estimate stays on the safe side.
[[nodiscard]] constexpr float terrainThreatFactor(TerrainFactors f) {
	const float gain = (f.speed - 1.f + f.damage / f.interval - 1.f) / 2.f;
	return gain > 0.f ? 1.f + TERRAIN_THREAT_WEIGHT * gain : 1.f;
}

// A kind's affinity is sane: never native and hostile to the same terrain, never native (or hostile) to both light
// and darkness.
[[nodiscard]] constexpr bool validAffinity(Terrains native, Terrains hostile) {
	const auto both = [](Terrains s) { return s.has(Terrain::Light) && s.has(Terrain::Darkness); };
	return !(native & hostile).any() && !both(native) && !both(hostile);
}

#endif
