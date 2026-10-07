#include "decor_scatter.h"
#include "monster_kinds.h"
#include "tile_defs.h"
#include "../core/gameplay_config.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace {
constexpr uint32_t DECOR_CHANCE_PERCENT = 30;
constexpr uint32_t DECAL_CHANCE_PERCENT = 35;
constexpr uint32_t DECAL_SALT = 0x51ed270bU;
constexpr uint32_t TORCH_SALT = 0x2c1b3c6dU;
constexpr uint32_t TORCH_CHANCE_PERCENT = 25;
constexpr uint32_t LADDER_SALT = 0x6a09e667U;
constexpr uint32_t SURFACE_SALT = 0x3c6ef372U;
constexpr int STRETCH_MIN = 3;	 // cells; a row of dressed stone is cut into stretches, painted or bare
constexpr int STRETCH_SPAN = 6;	 // lengths STRETCH_MIN .. STRETCH_MIN + STRETCH_SPAN - 1
constexpr int TORCH_MIN_GAP = 4; // cells between torches in a row
// Horizontal jitter per prop (tile units), from the extents decor.py prints, so props stay inside the tile.
constexpr float DECOR_JITTER[DECOR_COUNT] = {0.f,	0.12f, 0.06f, 0.1f, 0.f,  0.15f, 0.2f, 0.1f, 0.1f, 0.06f,
											 0.14f, 0.2f,  0.1f,  0.2f, 0.2f, 0.2f,	 0.2f, 0.1f, 0.f};

bool inBounds(int col, int row) { return LevelGrid::inBounds(col, row); }
int cellIndex(int col, int row) { return row * LEVEL_WIDTH + col; }
Tile at(const Tile* cells, int col, int row) { return cells[cellIndex(col, row)]; } // in bounds only
// A wall, or outside the level.
bool rockAt(const Tile* cells, int col, int row) { return !inBounds(col, row) || isWall(at(cells, col, row)); }
// Inside the level and a wall: the floor under a cell.
bool floorAt(const Tile* cells, int col, int row) { return inBounds(col, row) && isWall(at(cells, col, row)); }

// Weight of a prop or decal of `tier` at a level of `levelTier`: 0 if not unlocked, the newest tier double.
uint32_t tierWeight(int tier, int levelTier) {
	if (tier < 0 || tier > levelTier)
		return 0;
	return tier == levelTier ? 2 : 1;
}

// Index into `weights` (count of them) for the roll h: each entry as likely as its weight. -1 if all are 0.
int weightedPick(const uint32_t* weights, int count, uint32_t h) {
	uint32_t total = 0;
	for (int k = 0; k < count; k++)
		total += weights[k];
	if (total == 0)
		return -1;
	uint32_t roll = h % total;
	for (int k = 0; k < count; k++) {
		if (roll < weights[k])
			return k;
		roll -= weights[k];
	}
	return -1;
}

uint32_t hashName(const char* s) { // FNV-1a
	uint32_t h = 2166136261U;
	for (; *s != '\0'; s++) {
		h ^= static_cast<unsigned char>(*s);
		h *= 16777619U;
	}
	return h;
}

// 0..1 from the hash bits above the ones used for the choices.
float unit01(uint32_t h) { return static_cast<float>((h >> 8) % 1001) / 1000.f; }

uint32_t mix(uint32_t h) { // lowbias32 integer hash
	h ^= h >> 16;
	h *= 0x7feb352dU;
	h ^= h >> 15;
	h *= 0x846ca68bU;
	h ^= h >> 16;
	return h;
}

// A cell's own random number for this level's decoration pass (seed): the same cell, the same number.
uint32_t cellHash(uint32_t seed, int cell) { return mix(seed ^ mix(static_cast<uint32_t>(cell) + 0x9e3779b9U)); }

// A monster that lies in its coffin until the player comes near (the mummy).
bool entombed(int type) {
	const MonsterKind* kind = monsterKind(type);
	return kind != nullptr && kind->locomotion == Locomotion::Entombed;
}

int scatterProps(const Tile* cells, uint32_t seed, int tier, DecorCell* decor) {
	int placed = 0;
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++) {
			DecorCell& cell = decor[cellIndex(i, j)];
			cell = DecorCell{};
			const Tile tile = at(cells, i, j);
			if ((tile.type == MonsterSpawn && entombed(tile.attr)) || bossCoffinCell(cells, i, j)) {
				cell.type = DECOR_COFFIN;
				continue;
			}

			// Only empty cells the player can stand in (floor below). A slain boss's tile stays bare, as in his life.
			if (!isEmptyCell(tile) || slainBoss(tile) != 0 || !floorAt(cells, i, j - 1))
				continue;

			uint32_t h = cellHash(seed, cellIndex(i, j));
			if (h % 100 >= DECOR_CHANCE_PERCENT)
				continue;

			const bool wallLeft = rockAt(cells, i - 1, j);
			const bool wallRight = rockAt(cells, i + 1, j);
			// The web lies flat on the back wall, tucked into a side wall's corner if there is one: it needs a ceiling.
			const bool webFits = rockAt(cells, i, j + 1);

			// Thoth's variants count as one pick (the first), the variant comes after.
			uint32_t weights[DECOR_SCATTERED];
			for (int d = 0; d < DECOR_SCATTERED; d++)
				weights[d] = (isThoth(d) && d != DECOR_THOTH) || (d == DECOR_WEB && !webFits)
								 ? 0
								 : tierWeight(DECOR_TIERS[d], tier);
			h = mix(h);
			int type = weightedPick(weights, DECOR_SCATTERED, h);
			if (type < 0)
				continue;
			h = mix(h);
			if (type == DECOR_THOTH)
				type += static_cast<int>((h >> 3) % DECOR_THOTH_VARIANTS);
			cell.type = static_cast<int8_t>(type);
			if (type == DECOR_WEB)
				cell.mirror = wallLeft == wallRight ? (h & 1U) != 0 : wallRight;
			else
				cell.mirror = (h & 1U) != 0;
			cell.offsetX = (unit01(h) * 2.f - 1.f) * DECOR_JITTER[type];
			placed++;
		}
	return placed;
}

// Each vertical run of Ladder cells is one shaft with one style, keyed on its bottom cell: the bottom piece where it
// stands on the floor, the top piece in its highest cell, middle pieces in between, never the same one twice in a row.
int scatterLadders(const Tile* cells, uint32_t seed, LadderCell* ladder) {
	int shafts = 0;
	for (int i = 0; i < LEVEL_WIDTH; i++) {
		int style = 0;
		int below = -1; // middle piece of the cell below
		for (int j = 0; j < LEVEL_HEIGHT; j++) {
			LadderCell& cell = ladder[cellIndex(i, j)];
			cell = LadderCell{};
			if (at(cells, i, j).type != Ladder)
				continue;

			const bool first = !inBounds(i, j - 1) || at(cells, i, j - 1).type != Ladder;
			const bool last = !inBounds(i, j + 1) || at(cells, i, j + 1).type != Ladder;
			uint32_t h = cellHash(seed, cellIndex(i, j));
			if (first) {
				style = static_cast<int>(h % LADDER_STYLE_COUNT);
				below = -1;
				shafts++;
			}
			h = mix(h);

			int piece;
			if (first && (!inBounds(i, j - 1) || isSolidTile(at(cells, i, j - 1)))) // deep water under one in the water
				piece = LADDER_BOTTOM;
			else if (last)
				piece = LADDER_TOP;
			else {
				piece = static_cast<int>(h % LADDER_MID_COUNT);
				if (piece == below)
					piece = (piece + 1) % LADDER_MID_COUNT;
			}
			below = piece;
			cell.style = static_cast<int8_t>(style);
			cell.piece = static_cast<int8_t>(piece);
			cell.mirror = LADDER_MIRRORS[style] && ((h >> 8) & 1U) != 0;
		}
	}
	return shafts;
}

// Torches hang on the back wall at head height, spaced along each row. Cells with a fire of their own
// (brazier, oil lamp) and the gates and ladders are skipped.
int scatterTorches(const Tile* cells, uint32_t seed, const DecorCell* decor, bool* torch) {
	int placed = 0;
	for (int j = 0; j < LEVEL_HEIGHT; j++) {
		int lastTorch = -TORCH_MIN_GAP - 1;
		for (int i = 0; i < LEVEL_WIDTH; i++) {
			bool& cell = torch[cellIndex(i, j)];
			cell = false;

			const Tile tile = at(cells, i, j);
			const int8_t prop = decor[cellIndex(i, j)].type;
			if (isWall(tile) || !tileDef(tile.type).torch || prop == DECOR_BRAZIER || prop == DECOR_LAMP ||
				isThoth(prop) || i - lastTorch <= TORCH_MIN_GAP)
				continue;

			const uint32_t h = cellHash(seed, cellIndex(i, j));
			if (h % 100 >= TORCH_CHANCE_PERCENT)
				continue;
			cell = true;
			lastTorch = i;
			placed++;
		}
	}
	return placed;
}

// One decal at most per cell with a visible back wall. Runs after the props and torches so floor decals can avoid
// cells that already have a prop, free ones a torch.
int scatterDecals(const Tile* cells, uint32_t seed, int tier, const DecorLayout& layout, DecalCell* decal) {
	int placed = 0;
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++) {
			DecalCell& cell = decal[cellIndex(i, j)];
			cell = DecalCell{};

			const Tile tile = at(cells, i, j);
			if (isWall(tile) || !tileDef(tile.type).decals)
				continue;

			uint32_t h = cellHash(seed, cellIndex(i, j));
			if (h % 100 >= DECAL_CHANCE_PERCENT)
				continue;

			const bool ceiling = rockAt(cells, i, j + 1);
			const bool floor = floorAt(cells, i, j - 1) && !inHalfWater(tile) &&
							   layout.decor[cellIndex(i, j)].type < 0; // no dry grass under the water
			const bool free = !layout.torch[cellIndex(i, j)];		   // the torch covers the middle of the wall

			uint32_t weights[DECAL_COUNT];
			for (int d = 0; d < DECAL_COUNT; d++) {
				const DecalAnchor anchor = DECAL_DEFS[d].anchor;
				const bool fits = (anchor == DecalAnchor::Free && free) ||
								  (anchor == DecalAnchor::Ceiling && ceiling) ||
								  (anchor == DecalAnchor::Floor && floor);
				weights[d] = fits ? tierWeight(DECAL_TIERS[d], tier) : 0;
			}
			h = mix(h);
			const int type = weightedPick(weights, DECAL_COUNT, h);
			if (type < 0)
				continue;
			const DecalDef& def = DECAL_DEFS[type];
			const float half = def.size / 2.f;
			cell.type = static_cast<int8_t>(type);
			h = mix(h);
			cell.mirror = (h & 1U) != 0;
			cell.x = half + unit01(h) * (1.f - def.size);
			h = mix(h);
			switch (def.anchor) {
			case DecalAnchor::Ceiling:
				cell.y = 1.f - half;
				break;
			case DecalAnchor::Floor:
				cell.y = half;
				break;
			case DecalAnchor::Free: // around eye height, clear of the frieze near the top of the wall
				cell.y = std::max(half, 0.35f) + unit01(h) * std::max(0.f, 0.62f - std::max(half, 0.35f));
				break;
			}
			placed++;
		}
	return placed;
}

// Each row of open cells between two solid ones is rough rock (more often deeper down) or dressed stone. Dressed
// rows are cut into stretches, each painted or bare; a painted stretch ends in broken plaster where bare stone
// follows (stretches are at least STRETCH_MIN long, so only the one clipped by the row's end can be shorter, and it has
// stone on one side at most). Then a variant per cell, never the same uncommon one twice in a row.
// Rows of rough rock, dressed stone or painted plaster in the shares of the level's tier (ROUGH_PERCENT,
// PAINTED_PERCENT); the floors and ceilings that tier has unlocked.
void scatterSurfaces(const Tile* cells, uint32_t seed, int tier, SurfaceCell* surface, DecorCounts& counts) {
	enum Family : uint8_t { Painted, Stone, Rough };
	const uint32_t roughPercent = ROUGH_PERCENT[tier];
	const uint32_t paintedPercent = PAINTED_PERCENT[tier];
	Family family[LEVEL_WIDTH];
	int familyCells[3] = {};

	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int start = 0; start < LEVEL_WIDTH;) {
			if (isWall(at(cells, start, j))) {
				start++;
				continue;
			}
			int end = start;
			while (end < LEVEL_WIDTH && !isWall(at(cells, end, j)))
				end++;

			uint32_t h = cellHash(seed, cellIndex(start, j));
			const bool rough = h % 100 < roughPercent;
			for (int i = start; i < end;) {
				h = mix(h);
				const int len = STRETCH_MIN + static_cast<int>(h % STRETCH_SPAN);
				h = mix(h);
				const Family f = rough ? Rough : ((h >> 8) % 100 < paintedPercent ? Painted : Stone);
				for (int k = i; k < std::min(i + len, end); k++)
					family[k] = f;
				i += len;
			}

			int prevWall = -1;
			for (int i = start; i < end; i++) {
				SurfaceCell& cell = surface[cellIndex(i, j)];
				h = mix(h ^ static_cast<uint32_t>(i));
				const uint32_t roll = h % 100;
				cell.wallMirror = false; // a mirrored neighbour would show as a mirror line at the seam
				switch (family[i]) {
				case Painted:
					cell.ceiling = tier >= STAR_CEILING_TIER ? CEILING_STARS : CEILING_SLABS;
					if (i + 1 < end && family[i + 1] == Stone)
						cell.wall = WALL_PLASTER_BROKEN;
					else if (i > start && family[i - 1] == Stone) {
						cell.wall = WALL_PLASTER_BROKEN;
						cell.wallMirror = true;
					} else
						cell.wall = roll < 35 ? WALL_PLASTER_WORN : WALL_PLASTER;
					break;
				case Stone:
					cell.ceiling = CEILING_SLABS;
					cell.wall = roll < 25 ? WALL_STONE_CRACKED : (roll < 45 ? WALL_STONE_SAND : WALL_STONE);
					break;
				case Rough:
					cell.ceiling = CEILING_ROUGH;
					cell.wall = roll < 35 ? WALL_ROUGH_STRATA : WALL_ROUGH;
					break;
				}
				const bool common = cell.wall == WALL_PLASTER || cell.wall == WALL_STONE || cell.wall == WALL_ROUGH;
				if (!common && cell.wall == prevWall && cell.wall != WALL_PLASTER_BROKEN)
					cell.wall = family[i] == Painted ? WALL_PLASTER : (family[i] == Stone ? WALL_STONE : WALL_ROUGH);
				prevWall = cell.wall;
				familyCells[family[i]]++;

				// Sand on rough rock and in the cave, cracked floors from the worked tunnels, slabs from the tombs.
				h = mix(h);
				uint32_t floors[FLOOR_STYLE_COUNT] = {};
				floors[FLOOR_SAND] = family[i] == Rough ? 6 : 2;
				floors[FLOOR_CRACKED] = tier >= CRACKED_FLOOR_TIER ? 2 : 0;
				floors[FLOOR_SLABS] = tier >= SLAB_FLOOR_TIER && family[i] != Rough ? 5 : 0;
				cell.floor = weightedPick(floors, FLOOR_STYLE_COUNT, h);
			}
			start = end;
		}
	counts.painted = familyCells[Painted];
	counts.stone = familyCells[Stone];
	counts.rough = familyCells[Rough];
}
} // namespace

bool bossCoffinCell(const Tile* cells, int col, int row) {
	const Tile tile = at(cells, col, row);
	if (!isEmptyCell(tile) || slainBoss(tile) != 0 || !floorAt(cells, col, row - 1))
		return false;
	for (int k = -MINION_SUMMON_REACH; k <= MINION_SUMMON_REACH; k++) {
		if (!inBounds(col + k, row))
			continue;
		const Tile& t = cells[cellIndex(col + k, row)];
		const MonsterKind* boss = monsterKind(t.type == MonsterSpawn ? t.attr : slainBoss(t));
		if (boss != nullptr && boss->isBoss() && boss->boss.summon == Summon::Coffin)
			return true;
	}
	return false;
}

DecorCounts scatterDecor(const Tile* cells, const char* levelName, int depth, DecorLayout& layout) {
	const char* slash = strrchr(levelName, '/');
	const uint32_t seed = hashName(slash != nullptr ? slash + 1 : levelName);
	const int tier = decorTier(depth);
	DecorCounts counts;
	counts.props = scatterProps(cells, seed, tier, layout.decor);
	counts.torches = scatterTorches(cells, seed ^ TORCH_SALT, layout.decor, layout.torch);
	counts.decals = scatterDecals(cells, seed ^ DECAL_SALT, tier, layout, layout.decal);
	counts.ladderShafts = scatterLadders(cells, seed ^ LADDER_SALT, layout.ladder);
	scatterSurfaces(cells, seed ^ SURFACE_SALT, tier, layout.surface, counts);
	return counts;
}

int decorTierUsed(const Tile* cells, const DecorLayout& layout) {
	int used = -1;
	for (int c = 0; c < LEVEL_CELL_COUNT; c++) {
		if (layout.decor[c].type >= 0)
			used = std::max(used, static_cast<int>(DECOR_TIERS[layout.decor[c].type]));
		if (layout.decal[c].type >= 0)
			used = std::max(used, static_cast<int>(DECAL_TIERS[layout.decal[c].type]));
		if (isWall(cells[c]))
			continue;
		const SurfaceCell& s = layout.surface[c];
		if (s.ceiling == CEILING_STARS)
			used = std::max(used, STAR_CEILING_TIER);
		else if (s.wall <= WALL_PLASTER_BROKEN || s.floor == FLOOR_SLABS)
			used = std::max(used, SLAB_FLOOR_TIER);
		else if (s.wall <= WALL_STONE_SAND || s.floor == FLOOR_CRACKED)
			used = std::max(used, CRACKED_FLOOR_TIER);
		else
			used = std::max(used, 0);
	}
	return used;
}
