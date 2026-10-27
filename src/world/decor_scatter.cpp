#include "decor_scatter.h"
#include "campaign.h"
#include "zone_look.h"
#include "monster_kinds.h"
#include "tile_defs.h"
#include "../core/gameplay_config.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>

namespace {
constexpr uint32_t DECOR_CHANCE_PERCENT = 30;
constexpr uint32_t WATER_DECOR_CHANCE_PERCENT = 50; // water cells are fewer than dry ones: half of them planted
// Weight of each water prop (DECOR_WATER_FIRST on) at the bank (a neighbour in the row is not half water) and in the
// middle of a pool, times its tier's: reeds along the edge, leaves out on the water.
constexpr uint32_t WATER_BANK_WEIGHT[DECOR_WATER_COUNT] = {4, 1, 1, 1};
constexpr uint32_t WATER_MIDDLE_WEIGHT[DECOR_WATER_COUNT] = {1, 2, 3, 2};
constexpr uint32_t DECAL_CHANCE_PERCENT = 35;
constexpr uint32_t DECAL_SALT = 0x51ed270bU;
constexpr uint32_t TORCH_SALT = 0x2c1b3c6dU;
constexpr uint32_t LADDER_SALT = 0x6a09e667U;
constexpr uint32_t SURFACE_SALT = 0x3c6ef372U;
constexpr uint32_t TREASURE_SALT = 0xa54ff53aU;
constexpr int STRETCH_MIN = 3; // cells; a row of dressed stone is cut into stretches, painted or bare
constexpr int STRETCH_SPAN = 6; // lengths STRETCH_MIN .. STRETCH_MIN + STRETCH_SPAN - 1
constexpr int TORCH_MIN_GAP = 4; // cells between torches in a row
// The zones built of dressed stone (plan zone-looks: plaster, wet stone, basalt, granite): at most this many rows
// rough; the temples' plaster at least this many dressed ones painted.
constexpr uint32_t DRESSED_ROUGH_PERCENT = 15;
constexpr uint32_t PLASTER_PAINTED_PERCENT = 70;
// By ZoneStone: the dressed rows' walls (common, worn), floor and ceiling. Limestone and plaster have their own mix.
constexpr int ZONE_WALL[] = {WALL_STONE, WALL_STONE, WALL_WETSTONE, WALL_SANDSTONE, WALL_BASALT, WALL_GRANITE};
constexpr int ZONE_WALL_WORN[] = {
	WALL_STONE_CRACKED,
	WALL_STONE_CRACKED,
	WALL_WETSTONE_MOSS,
	WALL_SANDSTONE_WORN,
	WALL_BASALT_GLOW,
	WALL_GRANITE_WORN
};
constexpr int ZONE_FLOOR[] = {FLOOR_SLABS, FLOOR_SLABS, FLOOR_WETSTONE, FLOOR_SANDSTONE, FLOOR_BASALT, FLOOR_GRANITE};
constexpr int ZONE_CEILING[] =
	{CEILING_SLABS, CEILING_SLABS, CEILING_WETSTONE, CEILING_SANDSTONE, CEILING_BASALT, CEILING_GRANITE};
// Horizontal jitter per prop (tile units), from the extents decor.py prints, so props stay inside the tile.
constexpr float DECOR_JITTER[DECOR_COUNT] = {0.f,	0.12f, 0.06f, 0.1f, 0.f,  0.15f, 0.2f, 0.1f, 0.1f,
											 0.06f, 0.14f, 0.2f,  0.1f, 0.2f, 0.2f,	 0.2f, 0.2f, 0.1f,
											 0.f,	0.08f, 0.12f, 0.1f, 0.2f, 0.f,	 0.f,  0.f,	 0.f};

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

// A monster that stands among the decor figures until the player comes near (the living ushabti).
bool figurine(int type) {
	const MonsterKind* kind = monsterKind(type);
	return kind != nullptr && kind->locomotion == Locomotion::Figurine;
}

// A water prop in half water cell (i, j), plan water-decorations. Only where nothing else is: no monster spawn (a
// lurking crocodile stays in sight), no item, no fixture, no slain monster. False if it rolled none.
bool scatterWaterProp(const Tile* cells, uint32_t seed, int tier, int i, int j, DecorCell& cell) {
	const Tile tile = at(cells, i, j);
	if (tile.type != NoObject || slainMonster(tile) != 0 || hasFixture(tile))
		return false;
	uint32_t h = cellHash(seed, cellIndex(i, j));
	if (h % 100 >= WATER_DECOR_CHANCE_PERCENT)
		return false;

	const bool bank = !inBounds(i - 1, j) || !inHalfWater(at(cells, i - 1, j)) || !inBounds(i + 1, j) ||
					  !inHalfWater(at(cells, i + 1, j));
	uint32_t weights[DECOR_WATER_COUNT];
	for (int d = 0; d < DECOR_WATER_COUNT; d++)
		weights[d] = tierWeight(DECOR_TIERS[DECOR_WATER_FIRST + d], tier) *
					 (bank ? WATER_BANK_WEIGHT[d] : WATER_MIDDLE_WEIGHT[d]);
	h = mix(h);
	const int pick = weightedPick(weights, DECOR_WATER_COUNT, h);
	if (pick < 0)
		return false;
	const int type = DECOR_WATER_FIRST + pick;
	h = mix(h);
	cell.type = static_cast<int8_t>(type);
	cell.mirror = (h & 1U) != 0;
	cell.offsetX = (unit01(h) * 2.f - 1.f) * DECOR_JITTER[type];
	return true;
}

// A placed decal that grows from the floor (plan placed-decor): no random prop goes in front of it.
bool placedFloorDecal(const Tile& tile) {
	const int decal = tile.dressing.decal;
	return decal >= 0 && DECAL_DEFS[decal].anchor == DecalAnchor::Floor;
}

// A prop with its own fire or a crown up to the torch, or the Stargate over it: none goes with a wall torch
// (scatterTorches).
bool torchless(int prop) {
	return prop == DECOR_BRAZIER || prop == DECOR_LAMP || isThoth(prop) || prop == DECOR_STARGATE;
}

int scatterProps(const Tile* cells, uint32_t seed, int tier, const ZoneLook& look, DecorCell* decor) {
	int placed = 0;
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++) {
			DecorCell& cell = decor[cellIndex(i, j)];
			cell = DecorCell{};
			const Tile tile = at(cells, i, j);
			if (entombed(spawnType(tile)) || bossCoffinCell(cells, i, j)) {
				cell.type = static_cast<int8_t>(isBossMonster(spawnType(tile)) ? DECOR_COFFIN_LARGE : DECOR_COFFIN);
				continue;
			}
			if (tile.type == Page) { // the earlier expedition's remains, its page on them; they stay once it is taken
				cell.type = static_cast<int8_t>(DECOR_SKELETON);
				continue;
			}
			if (figurine(spawnType(tile))) { // its fellows, as the decor stands elsewhere (mirror and jitter: none)
				cell.type = static_cast<int8_t>(DECOR_USHABTI);
				continue;
			}
			// The decor layer (plan placed-decor): the designer's prop, whatever the tiers and the cell's contents.
			if (const Dressing& dressed = tile.dressing; hasPlacedProp(dressed)) {
				if (dressed.prop == PLACED_BARE)
					continue;
				const uint32_t h = mix(mix(cellHash(seed, cellIndex(i, j))));
				cell.type = static_cast<int8_t>(dressed.prop);
				cell.mirror = dressed.propMirror;
				cell.offsetX = (unit01(h) * 2.f - 1.f) * DECOR_JITTER[dressed.prop];
				placed++;
				continue;
			}

			if (inHalfWater(tile)) {
				placed += scatterWaterProp(cells, seed, tier, i, j, cell) ? 1 : 0;
				continue;
			}
			// Only empty cells the player can stand in (floor below). A slain monster's tile stays bare, as in its
			// life, a placed floor decal free.
			if (!isEmptyCell(tile) || slainMonster(tile) != 0 || !floorAt(cells, i, j - 1) || placedFloorDecal(tile))
				continue;

			uint32_t h = cellHash(seed, cellIndex(i, j));
			if (h % 100 >= DECOR_CHANCE_PERCENT)
				continue;

			const bool wallLeft = rockAt(cells, i - 1, j);
			const bool wallRight = rockAt(cells, i + 1, j);
			// The web lies flat on the back wall, tucked into a side wall's corner if there is one: it needs a ceiling.
			const bool webFits = rockAt(cells, i, j + 1);

			// Thoth's variants count as one pick (the first), the variant comes after. Under a placed torch no fire or
			// crown of its own.
			const bool torch = tile.dressing.torch == TorchPlacing::Torch;
			uint32_t weights[DECOR_SCATTERED];
			for (int d = 0; d < DECOR_SCATTERED; d++)
				weights[d] = (isThoth(d) && d != DECOR_THOTH) || (d == DECOR_WEB && !webFits) || (torch && torchless(d))
								 ? 0
								 : tierWeight(DECOR_TIERS[d], tier) * look.props[d];
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

// Every open cell gets a look, not only the treasure cells: a chest taken before a save still stands there, opened,
// after the load (its tile is empty then).
int scatterTreasures(const Tile* cells, uint32_t seed, int tier, TreasureLook* treasure) {
	int chests = 0;
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++) {
			TreasureLook& look = treasure[cellIndex(i, j)];
			if (rockAt(cells, i, j)) {
				look = TreasureLook{};
				continue;
			}
			uint32_t h = cellHash(seed, cellIndex(i, j));
			uint32_t kinds[CONTAINER_COUNT];
			for (int k = 0; k < CONTAINER_COUNT; k++)
				kinds[k] = static_cast<uint32_t>(CONTAINER_PERCENT[tier][k]);
			const int kind = weightedPick(kinds, CONTAINER_COUNT, h);
			uint32_t stances[STANCE_COUNT];
			for (int k = 0; k < STANCE_COUNT; k++) {
				const StanceDef& st = STANCES[k];
				stances[k] = st.wallSide == 0 || rockAt(cells, i + st.wallSide, j) ? static_cast<uint32_t>(st.weight)
																				   : 0;
			}
			const StanceDef& stance = STANCES[weightedPick(stances, STANCE_COUNT, mix(h))];
			look = {static_cast<Container>(kind), stance.yaw, stance.x, stance.z};
			chests += isTreasure(at(cells, i, j)) ? 1 : 0;
		}
	return chests;
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

// A torch the decal layer places within TORCH_MIN_GAP after column col of row (plan placed-decor).
bool placedTorchAhead(const Tile* cells, int col, int row) {
	for (int k = col + 1; k <= col + TORCH_MIN_GAP && k < LEVEL_WIDTH; k++)
		if (at(cells, k, row).dressing.torch == TorchPlacing::Torch)
			return true;
	return false;
}

// Torches hang on the back wall at head height, spaced along each row. Cells with a fire of their own (brazier, oil
// lamp), the gates and ladders and deep water (plan no-torches-under-water) are skipped; over half water the flame
// clears the surface. The decal layer's torches (plan placed-decor) hang where it says, whatever the rules; the random
// ones keep the gap to them.
int scatterTorches(const Tile* cells, uint32_t seed, uint32_t torchPercent, const DecorCell* decor, bool* torch) {
	int placed = 0;
	for (int j = 0; j < LEVEL_HEIGHT; j++) {
		int lastTorch = -TORCH_MIN_GAP - 1;
		for (int i = 0; i < LEVEL_WIDTH; i++) {
			bool& cell = torch[cellIndex(i, j)];
			cell = false;

			const Tile tile = at(cells, i, j);
			if (tile.dressing.torch != TorchPlacing::Unset) {
				cell = tile.dressing.torch == TorchPlacing::Torch;
				lastTorch = cell ? i : lastTorch;
				placed += cell ? 1 : 0;
				continue;
			}
			const int8_t prop = decor[cellIndex(i, j)].type;
			const int decal = tile.dressing.decal; // a placed free decal takes the middle of the wall
			if (isWall(tile) || tile.structure == Structure::DeepWater || !tileDef(tile.type).torch ||
				!tileDef(tile.fixture.type).torch || torchless(prop) || i - lastTorch <= TORCH_MIN_GAP ||
				(decal >= 0 && DECAL_DEFS[decal].anchor == DecalAnchor::Free) || placedTorchAhead(cells, i, j))
				continue;

			const uint32_t h = cellHash(seed, cellIndex(i, j));
			if (h % 100 >= torchPercent)
				continue;
			cell = true;
			lastTorch = i;
			placed++;
		}
	}
	return placed;
}

// Decal type in cell, its x from the roll h, its y from its anchor (and the next roll for a free one).
void placeDecal(DecalCell& cell, int type, bool mirror, uint32_t h) {
	const DecalDef& def = DECAL_DEFS[type];
	const float half = def.size / 2.f;
	cell.type = static_cast<int8_t>(type);
	cell.mirror = mirror;
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
}

// One decal at most per cell with a visible back wall. Runs after the props and torches so floor decals can avoid
// cells that already have a prop, free ones a torch.
int scatterDecals(
	const Tile* cells,
	uint32_t seed,
	int tier,
	const ZoneLook& look,
	const DecorLayout& layout,
	DecalCell* decal
) {
	int placed = 0;
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++) {
			DecalCell& cell = decal[cellIndex(i, j)];
			cell = DecalCell{};

			const Tile tile = at(cells, i, j);
			const int dressed = tile.dressing.decal;
			if (dressed == PLACED_BARE)
				continue;
			uint32_t h = cellHash(seed, cellIndex(i, j));
			if (dressed >= 0) { // the decal layer's (plan placed-decor): whatever the tiers and the rules
				placeDecal(cell, dressed, tile.dressing.decalMirror, h);
				placed++;
				continue;
			}
			if (isWall(tile) || !tileDef(tile.type).decals || !tileDef(tile.fixture.type).decals)
				continue;
			if (h % 100 >= DECAL_CHANCE_PERCENT)
				continue;

			const bool ceiling = rockAt(cells, i, j + 1);
			const bool floor = floorAt(cells, i, j - 1) && !inHalfWater(tile) &&
							   layout.decor[cellIndex(i, j)].type < 0; // no dry grass under the water
			const bool free = !layout.torch[cellIndex(i, j)]; // the torch covers the middle of the wall

			uint32_t weights[DECAL_COUNT];
			for (int d = 0; d < DECAL_COUNT; d++) {
				const DecalAnchor anchor = DECAL_DEFS[d].anchor;
				const bool fits = (anchor == DecalAnchor::Free && free) ||
								  (anchor == DecalAnchor::Ceiling && ceiling) ||
								  (anchor == DecalAnchor::Floor && floor);
				weights[d] = fits ? tierWeight(DECAL_TIERS[d], tier) * look.decals[d] : 0;
			}
			h = mix(h);
			const int type = weightedPick(weights, DECAL_COUNT, h);
			if (type < 0)
				continue;
			h = mix(h);
			placeDecal(cell, type, (h & 1U) != 0, h);
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
// The zone's stone (plan zone-looks): limestone as above, plaster the same painted more often, the other stones dress
// every row of their own, no plaster.
void scatterSurfaces(
	const Tile* cells,
	uint32_t seed,
	int tier,
	ZoneStone stone,
	SurfaceCell* surface,
	DecorCounts& counts
) {
	enum Family : uint8_t { Painted, Stone, Rough };
	const bool desert = stone == ZoneStone::Sandstone;
	const bool limestone = stone == ZoneStone::Limestone || stone == ZoneStone::Plaster;
	const bool built = stone != ZoneStone::Limestone && stone != ZoneStone::Sandstone;
	const uint32_t roughPercent = built ? std::min(ROUGH_PERCENT[tier], DRESSED_ROUGH_PERCENT) : ROUGH_PERCENT[tier];
	const uint32_t paintedPercent = !limestone ? 0
									: stone == ZoneStone::Plaster
										? std::max(PAINTED_PERCENT[tier], PLASTER_PAINTED_PERCENT)
										: PAINTED_PERCENT[tier];
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
					if (desert) // a crack with daylight through it where rock is above and below, else crust or sand
						cell.ceiling = isWall(at(cells, i, j + 1)) && isWall(at(cells, i, j - 1)) &&
											   (h >> 12) % LIGHT_SHAFT_EVERY == 0
										   ? CEILING_SANDSTONE_CRACKED
									   : roll % 3 == 0 ? CEILING_SANDSTONE_SANDY
													   : CEILING_SANDSTONE;
					cell.wall = roll < 25 ? WALL_STONE_CRACKED : (roll < 45 ? WALL_STONE_SAND : WALL_STONE);
					if (!limestone && !desert)
						cell.ceiling = ZONE_CEILING[static_cast<int>(stone)];
					if (!limestone)
						cell.wall = roll < 35 ? ZONE_WALL_WORN[static_cast<int>(stone)]
											  : ZONE_WALL[static_cast<int>(stone)];
					break;
				case Rough:
					cell.ceiling = CEILING_ROUGH;
					cell.wall = roll < 35 ? WALL_ROUGH_STRATA : WALL_ROUGH;
					break;
				}
				const int dressed = ZONE_WALL[static_cast<int>(stone)];
				const bool common = cell.wall == WALL_PLASTER || cell.wall == dressed || cell.wall == WALL_ROUGH;
				if (!common && cell.wall == prevWall && cell.wall != WALL_PLASTER_BROKEN)
					cell.wall = family[i] == Painted ? WALL_PLASTER : (family[i] == Stone ? dressed : WALL_ROUGH);
				prevWall = cell.wall;
				familyCells[family[i]]++;

				// Sand on rough rock and in the cave, cracked floors from the worked tunnels, slabs from the tombs.
				h = mix(h);
				uint32_t floors[FLOOR_STYLE_COUNT] = {};
				floors[FLOOR_SAND] = family[i] == Rough ? 6 : 2;
				floors[FLOOR_CRACKED] = tier >= CRACKED_FLOOR_TIER ? 2 : 0;
				floors[FLOOR_SLABS] = tier >= SLAB_FLOOR_TIER && family[i] != Rough ? 5 : 0;
				if (!limestone) { // the stone's own slabs and sand; less sand on wet stone, none on its slabs
					const bool wet = stone == ZoneStone::WetStone;
					floors[FLOOR_SAND] = family[i] == Rough ? (wet ? 3 : 6) : (wet ? 0 : (desert ? 3 : 1));
					floors[FLOOR_CRACKED] = 0;
					floors[FLOOR_SLABS] = 0;
					floors[ZONE_FLOOR[static_cast<int>(stone)]] = family[i] == Rough ? 0 : 5;
				}
				cell.floor = weightedPick(floors, FLOOR_STYLE_COUNT, h);
			}
			start = end;
		}
	// Where a stone floor meets a sand one on the same row, the sand spills over (both dry, on rock).
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++) {
			auto floored = [&](int col) {
				return inBounds(col, j) && !isWall(at(cells, col, j)) && !inHalfWater(at(cells, col, j)) &&
					   floorAt(cells, col, j - 1);
			};
			SurfaceCell& cell = surface[cellIndex(i, j)];
			const bool stone = floored(i) && cell.floor != FLOOR_SAND;
			cell.sandLeft = stone && floored(i - 1) && surface[cellIndex(i - 1, j)].floor == FLOOR_SAND;
			cell.sandRight = stone && floored(i + 1) && surface[cellIndex(i + 1, j)].floor == FLOOR_SAND;
		}
	counts.painted = familyCells[Painted];
	counts.stone = familyCells[Stone];
	counts.rough = familyCells[Rough];
}
} // namespace

bool bossCoffinCell(const Tile* cells, int col, int row) {
	const Tile tile = at(cells, col, row);
	if (!isEmptyCell(tile) || slainMonster(tile) != 0 || !floorAt(cells, col, row - 1))
		return false;
	for (int k = -MINION_SUMMON_REACH; k <= MINION_SUMMON_REACH; k++) {
		if (!inBounds(col + k, row))
			continue;
		const Tile& t = cells[cellIndex(col + k, row)];
		const MonsterKind* boss = monsterKind(spawnType(t));
		if (boss != nullptr && boss->isBoss() && boss->boss.summon == Summon::Coffin)
			return true;
	}
	return false;
}

DecorCounts scatterDecor(const Tile* cells, const char* levelName, int depth, int zone, DecorLayout& layout) {
	const char* slash = strrchr(levelName, '/');
	const uint32_t seed = hashName(slash != nullptr ? slash + 1 : levelName);
	const int tier = decorTier(depth);
	DecorCounts counts;
	std::fill(std::begin(layout.doused), std::end(layout.doused), false);
	const ZoneLook& look = levelLook(zone); // plan zone-looks: the zone's decor set and torches
	counts.props = scatterProps(cells, seed, tier, look, layout.decor);
	counts.torches = scatterTorches(cells, seed ^ TORCH_SALT, look.torchPercent, layout.decor, layout.torch);
	counts.decals = scatterDecals(cells, seed ^ DECAL_SALT, tier, look, layout, layout.decal);
	counts.ladderShafts = scatterLadders(cells, seed ^ LADDER_SALT, layout.ladder);
	const ZoneStone stone = isZone(zone) ? zoneInfo(static_cast<ZoneId>(zone)).stone : ZoneStone::Limestone;
	scatterSurfaces(cells, seed ^ SURFACE_SALT, tier, stone, layout.surface, counts);
	counts.treasures = scatterTreasures(cells, seed ^ TREASURE_SALT, tier, layout.treasure);
	return counts;
}

int decorTierUsed(const Tile* cells, const DecorLayout& layout) {
	int used = -1;
	for (int c = 0; c < LEVEL_CELL_COUNT; c++) {
		if (layout.decor[c].type >= 0 && !hasPlacedProp(cells[c].dressing))
			used = std::max(used, static_cast<int>(DECOR_TIERS[layout.decor[c].type]));
		if (layout.decal[c].type >= 0 && !hasPlacedDecal(cells[c].dressing))
			used = std::max(used, static_cast<int>(DECAL_TIERS[layout.decal[c].type]));
		if (isWall(cells[c]))
			continue;
		const SurfaceCell& s = layout.surface[c];
		if (s.ceiling == CEILING_STARS)
			used = std::max(used, STAR_CEILING_TIER);
		else if (s.wall <= WALL_PLASTER_BROKEN || s.floor == FLOOR_SLABS)
			used = std::max(used, SLAB_FLOOR_TIER);
		else if (s.wall <= WALL_STONE_SAND || s.wall >= WALL_SANDSTONE || s.floor == FLOOR_CRACKED ||
				 s.floor >= FLOOR_SANDSTONE)
			used = std::max(used, CRACKED_FLOOR_TIER);
		else
			used = std::max(used, 0);
	}
	return used;
}

namespace {
// Flame origins in prop space (tile units, x before mirroring), from the geometry in decor.py.
constexpr float BRAZIER_FIRE[3] = {0.f, 0.22f, 0.16f}; // on the charcoal
constexpr float LAMP_FIRE[3] = {-0.256f, 0.05f, 0.307f}; // oil lamp wick
constexpr float TORCH_FIRE[3] = {0.f, 0.68f, 0.098f}; // top of the torch head
} // namespace

int flamesAt(const DecorLayout& layout, int cell, Flame (&out)[MAX_CELL_FLAMES], bool withDoused) {
	int n = 0;
	auto seed = static_cast<uint32_t>(cell) * 2654435761U;
	auto at = [&](const float* p, float offsetX, bool mirror, FlameKind kind) {
		out[n] = {offsetX + (mirror ? -p[0] : p[0]), p[1], p[2], kind, seed + static_cast<uint32_t>(n)};
		n++;
	};

	const DecorCell& decor = layout.decor[cell];
	const bool lit = withDoused || !layout.doused[cell];
	if (decor.type == DECOR_BRAZIER)
		at(BRAZIER_FIRE, decor.offsetX, decor.mirror, FlameKind::Brazier);
	else if (decor.type == DECOR_LAMP && lit)
		at(LAMP_FIRE, decor.offsetX, decor.mirror, FlameKind::OilLamp);
	if (layout.torch[cell] && lit)
		at(TORCH_FIRE, 0.f, false, FlameKind::Torch);
	return n;
}
