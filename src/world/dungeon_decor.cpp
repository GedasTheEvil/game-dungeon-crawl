#include "dungeon.h"
#include "tile_defs.h"
#include "../state/assets.h"
#include "../entities/player.h"
#include "item_bag.h"
#include "journal.h"
#include "rng.h"
#include "world_events.h"
#include "../graphics/render_config.h"
#include "../graphics/fire.h"
#include "../graphics/lighting.h"
#include "../core/logger.h"
#include "../core/gameplay_config.h"
#include <GL/gl.h>
#include <cstdint>
#include <algorithm>
#include <cstring>

namespace {
constexpr uint32_t DECOR_CHANCE_PERCENT = 30;
constexpr uint32_t DECAL_CHANCE_PERCENT = 35;
constexpr uint32_t DECAL_SALT = 0x51ed270bU;
constexpr uint32_t TORCH_SALT = 0x2c1b3c6dU;
constexpr uint32_t TORCH_CHANCE_PERCENT = 25;
constexpr uint32_t LADDER_SALT = 0x6a09e667U;
constexpr uint32_t SURFACE_SALT = 0x3c6ef372U;
// Share of the rows of open cells that are rough-hewn rock, from the first level to the deepest.
constexpr uint32_t ROUGH_PERCENT_FIRST = 15;
constexpr uint32_t ROUGH_PERCENT_STEP = 4; // per level
constexpr uint32_t ROUGH_PERCENT_MAX = 60;
constexpr uint32_t PAINTED_PERCENT = 45; // of the stretches of a dressed-stone row
constexpr int STRETCH_MIN = 3;			 // cells; a row of dressed stone is cut into stretches, painted or bare
constexpr int STRETCH_SPAN = 6;			 // lengths STRETCH_MIN .. STRETCH_MIN + STRETCH_SPAN - 1
constexpr int TORCH_MIN_GAP = 4;		 // cells between torches in a row
// Flame origins in prop space (tile units, x before mirroring), from the geometry in decor.py.
constexpr float BRAZIER_FIRE[3] = {0.f, 0.22f, 0.16f};	 // on the charcoal
constexpr float LAMP_FIRE[3] = {-0.256f, 0.05f, 0.307f}; // oil lamp wick
constexpr float TORCH_FIRE[3] = {0.f, 0.68f, 0.098f};	 // top of the torch head
constexpr float LIGHT_LIFT = 4.f;						 // lights sit above and in front of the flame (world units)
// Horizontal jitter per prop (tile units), from the extents decor.py prints, so props stay inside the tile.
constexpr float DECOR_JITTER[DECOR_COUNT] = {0.f,	0.12f, 0.06f, 0.1f, 0.f,  0.15f, 0.2f, 0.1f, 0.1f, 0.06f,
											 0.14f, 0.2f,  0.1f,  0.2f, 0.2f, 0.2f,	 0.2f, 0.1f, 0.f};
// The props a cell picks from: Thoth's variants count as one.
constexpr int DECOR_PICKS = DECOR_SCATTERED - (DECOR_THOTH_VARIANTS - 1);

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
} // namespace

// An empty floor cell within MINION_SUMMON_REACH of a boss on its row whose minions climb out of coffins
// (Summon::Coffin), alive or slain (slainBoss): a coffin for them stands there.
bool Dungeon::bossCoffin(int i, int j) const {
	if (!isEmptyCell(MapAt(i, j)) || slainBoss(MapAt(i, j)) != 0 || !IsInBounds(i, j - 1) || !isWall(MapAt(i, j - 1)))
		return false;
	for (int k = -MINION_SUMMON_REACH; k <= MINION_SUMMON_REACH; k++) {
		if (!IsInBounds(i + k, j))
			continue;
		const Tile& t = MapAt(i + k, j);
		const int boss = t.type == MonsterSpawn ? t.attr : slainBoss(t);
		if (boss >= 1 && boss <= MONSTER_TYPE_MAX && sim.assets->monsterTypes[boss].boss.summon == Summon::Coffin)
			return true;
	}
	return false;
}

int Dungeon::CoffinCount() const {
	int n = 0;
	for (const DecorCell& cell : decor)
		if (cell.type == DECOR_COFFIN)
			n++;
	return n;
}

// Every cell rolls independently from (level name, cell index), so the layout is the same on every
// load of a level and does not depend on the rest of the map.
bool Dungeon::PlaceDecor(int col, int row, int type) {
	if (!IsInBounds(col, row) || type < 0 || type >= DECOR_COUNT)
		return false;
	decor[MapIndex(col, row)] = DecorCell{static_cast<int8_t>(type), false, 0.f};
	return true;
}

void Dungeon::scatterDecorations(const char* levelName) {
	const char* slash = strrchr(levelName, '/');
	uint32_t seed = hashName(slash != nullptr ? slash + 1 : levelName);
	int placed = 0;

	for (int j = 0; j < MAP_HEIGHT; j++)
		for (int i = 0; i < MAP_WIDTH; i++) {
			DecorCell& cell = decor[MapIndex(i, j)];
			cell = DecorCell{};
			if ((MapAt(i, j).type == MonsterSpawn && MapAt(i, j).attr == MonsterMummy) || bossCoffin(i, j)) {
				cell.type = DECOR_COFFIN;
				continue;
			}

			// Only empty cells the player can stand in (floor below). A slain boss's tile stays bare, as in his life.
			if (!isEmptyCell(MapAt(i, j)) || slainBoss(MapAt(i, j)) != 0 || !IsInBounds(i, j - 1) ||
				!isWall(MapAt(i, j - 1)))
				continue;

			uint32_t h = cellHash(seed, MapIndex(i, j));
			if (h % 100 >= DECOR_CHANCE_PERCENT)
				continue;

			bool wallLeft = !IsInBounds(i - 1, j) || isWall(MapAt(i - 1, j));
			bool wallRight = !IsInBounds(i + 1, j) || isWall(MapAt(i + 1, j));
			bool ceiling = !IsInBounds(i, j + 1) || isWall(MapAt(i, j + 1));
			bool webFits = ceiling; // lies flat on the back wall, tucked into a side wall's corner if there is one

			h = mix(h);
			int type = webFits ? static_cast<int>(h % DECOR_PICKS) : 1 + static_cast<int>(h % (DECOR_PICKS - 1));
			h = mix(h);
			if (type == DECOR_THOTH)
				type += static_cast<int>((h >> 3) % DECOR_THOTH_VARIANTS);
			else if (type > DECOR_THOTH)
				type += DECOR_THOTH_VARIANTS - 1;
			cell.type = static_cast<int8_t>(type);
			if (type == DECOR_WEB)
				cell.mirror = wallLeft == wallRight ? (h & 1U) != 0 : wallRight;
			else
				cell.mirror = (h & 1U) != 0;
			cell.offsetX = (unit01(h) * 2.f - 1.f) * DECOR_JITTER[type];
			placed++;
		}
	LOG_INFOF("world", "Decorations in %s: %d", levelName, placed);

	scatterTorches(seed ^ TORCH_SALT);
	scatterDecals(seed ^ DECAL_SALT);
	scatterLadders(seed ^ LADDER_SALT);
	scatterSurfaces(seed ^ SURFACE_SALT);
}
//======================================================================================
// Each vertical run of Ladder cells is one shaft with one style, keyed on its bottom cell: the bottom piece where it
// stands on the floor, the top piece in its highest cell, middle pieces in between, never the same one twice in a row.
void Dungeon::scatterLadders(uint32_t seed) {
	int shafts = 0;

	for (int i = 0; i < MAP_WIDTH; i++) {
		int style = 0;
		int below = -1; // middle piece of the cell below
		for (int j = 0; j < MAP_HEIGHT; j++) {
			LadderCell& cell = ladder[MapIndex(i, j)];
			cell = LadderCell{};
			if (MapAt(i, j).type != Ladder)
				continue;

			bool first = !IsInBounds(i, j - 1) || MapAt(i, j - 1).type != Ladder;
			bool last = !IsInBounds(i, j + 1) || MapAt(i, j + 1).type != Ladder;
			uint32_t h = cellHash(seed, MapIndex(i, j));
			if (first) {
				style = static_cast<int>(h % LADDER_STYLE_COUNT);
				below = -1;
				shafts++;
			}
			h = mix(h);

			int piece;
			if (first && (!IsInBounds(i, j - 1) || isSolidTile(MapAt(i, j - 1)))) // deep water under one in the water
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
	LOG_INFOF("world", "Ladder shafts: %d", shafts);
}
//======================================================================================
// Torches hang on the back wall at head height, spaced along each row. Cells with a fire of their own
// (brazier, oil lamp) and the gates and ladders are skipped.
void Dungeon::scatterTorches(uint32_t seed) {
	int placed = 0;

	for (int j = 0; j < MAP_HEIGHT; j++) {
		int lastTorch = -TORCH_MIN_GAP - 1;
		for (int i = 0; i < MAP_WIDTH; i++) {
			bool& cell = torch[MapIndex(i, j)];
			cell = false;

			const Tile tile = MapAt(i, j);
			int8_t prop = decor[MapIndex(i, j)].type;
			if (isWall(tile) || !tileDef(tile.type).torch || prop == DECOR_BRAZIER || prop == DECOR_LAMP ||
				isThoth(prop) || i - lastTorch <= TORCH_MIN_GAP)
				continue;

			uint32_t h = cellHash(seed, MapIndex(i, j));
			if (h % 100 >= TORCH_CHANCE_PERCENT)
				continue;
			cell = true;
			lastTorch = i;
			placed++;
		}
	}
	LOG_INFOF("world", "Torches: %d", placed);
}
//======================================================================================
// One decal at most per cell with a visible back wall. Runs after the props so floor decals can
// avoid cells that already have a prop.
void Dungeon::scatterDecals(uint32_t seed) {
	int placed = 0;

	for (int j = 0; j < MAP_HEIGHT; j++)
		for (int i = 0; i < MAP_WIDTH; i++) {
			DecalCell& cell = decal[MapIndex(i, j)];
			cell = DecalCell{};

			const Tile tile = MapAt(i, j);
			if (isWall(tile) || !tileDef(tile.type).decals)
				continue;

			uint32_t h = cellHash(seed, MapIndex(i, j));
			if (h % 100 >= DECAL_CHANCE_PERCENT)
				continue;

			bool ceiling = !IsInBounds(i, j + 1) || isWall(MapAt(i, j + 1));
			bool floor = IsInBounds(i, j - 1) && isWall(MapAt(i, j - 1)) && !inHalfWater(tile) &&
						 decor[MapIndex(i, j)].type < 0; // no dry grass under the water
			bool free = !torch[MapIndex(i, j)];			 // the torch covers the middle of the wall

			int fits[DECAL_COUNT];
			int fitCount = 0;
			for (int d = 0; d < DECAL_COUNT; d++) {
				DecalAnchor anchor = DECAL_DEFS[d].anchor;
				if ((anchor == DecalAnchor::Free && free) || (anchor == DecalAnchor::Ceiling && ceiling) ||
					(anchor == DecalAnchor::Floor && floor))
					fits[fitCount++] = d;
			}
			if (fitCount == 0)
				continue;

			h = mix(h);
			int type = fits[h % static_cast<uint32_t>(fitCount)];
			const DecalDef& def = DECAL_DEFS[type];
			float half = def.size / 2.f;
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
	LOG_INFOF("world", "Wall decals: %d", placed);
}
//======================================================================================
void Dungeon::drawDecorTile(int i, int j) {
	const DecorCell& cell = decor[MapIndex(i, j)];
	if (cell.type < 0)
		return;

	AnimatedModel* model = sim.assets->decor.model[cell.type].get();
	if (model == nullptr)
		return;

	glPushMatrix();
	glTranslatef(RenderConfig::TILE_HALF + cell.offsetX * RenderConfig::TILE_SIZE, 0, -RenderConfig::TILE_SIZE);
	glScalef(cell.mirror ? -RenderConfig::TILE_SIZE : RenderConfig::TILE_SIZE, RenderConfig::TILE_SIZE,
			 RenderConfig::TILE_SIZE);

	// Textured only (lighting is baked in); the toon pass would wash out dark details.
	sim.assets->decor.tex[cell.type].Bind();
	model->Show(); // no face culling, so the mirrored winding does not matter
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawDecalTile(int i, int j) {
	const DecalCell& cell = decal[MapIndex(i, j)];
	if (cell.type < 0)
		return;

	float tile = RenderConfig::TILE_SIZE;
	float half = DECAL_DEFS[cell.type].size * tile / 2.f;
	float cx = cell.x * tile;
	float cy = cell.y * tile;
	float z = -tile + 0.05f; // just in front of the back wall

	// Atlas cell: row 0 is the top of the image, which the loader puts at t = 1.
	float step = 1.f / static_cast<float>(DECAL_ATLAS_GRID);
	float u0 = static_cast<float>(cell.type % DECAL_ATLAS_GRID) * step;
	int row = cell.type / DECAL_ATLAS_GRID;
	float v1 = 1.f - static_cast<float>(row) * step;
	float u1 = u0 + step;
	float v0 = v1 - step;
	if (cell.mirror)
		std::swap(u0, u1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glColor3f(1, 1, 1);
	sim.assets->decor.decalTex.Bind();
	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glTexCoord2f(u0, v0);
	glVertex3f(cx - half, cy - half, z);
	glTexCoord2f(u1, v0);
	glVertex3f(cx + half, cy - half, z);
	glTexCoord2f(u1, v1);
	glVertex3f(cx + half, cy + half, z);
	glTexCoord2f(u0, v1);
	glVertex3f(cx - half, cy + half, z);
	glEnd();
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}
//======================================================================================
void Dungeon::drawTorchTile(int i, int j) {
	AnimatedModel* model = sim.assets->decor.torch.get();
	if (!torch[MapIndex(i, j)] || model == nullptr)
		return;

	glPushMatrix();
	glTranslatef(RenderConfig::TILE_HALF, 0, -RenderConfig::TILE_SIZE);
	glScalef(RenderConfig::TILE_SIZE, RenderConfig::TILE_SIZE, RenderConfig::TILE_SIZE);
	sim.assets->decor.torchTex.Bind();
	model->Show();
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawLadderTile(int i, int j) {
	const LadderCell& cell = ladder[MapIndex(i, j)];
	if (cell.style < 0)
		return;
	AnimatedModel* model = sim.assets->decor.ladder[cell.style][cell.piece].get();
	if (model == nullptr)
		return;

	glPushMatrix();
	glTranslatef(RenderConfig::TILE_HALF, 0, -RenderConfig::TILE_SIZE);
	glScalef(cell.mirror ? -RenderConfig::TILE_SIZE : RenderConfig::TILE_SIZE, RenderConfig::TILE_SIZE,
			 RenderConfig::TILE_SIZE);
	sim.assets->decor.ladderTex[cell.style][cell.piece].Bind();
	model->Show(); // textured only, like the props
	glPopMatrix();
}
//======================================================================================
struct Dungeon::FlameSource {
	float x, y, z; // tile space (the frame drawDecorTile starts from), world units
	const Lighting::LightDef* light;
	const FireStyle* fire;
	uint32_t seed;
};

// A cell holds at most a prop fire and a torch.
int Dungeon::flamesAt(int i, int j, FlameSource* out) const {
	int n = 0;
	auto seed = static_cast<uint32_t>(MapIndex(i, j)) * 2654435761U;
	auto at = [&](const float* p, float offsetX, bool mirror, const Lighting::LightDef& light, const FireStyle& fire) {
		float t = RenderConfig::TILE_SIZE;
		out[n] = {RenderConfig::TILE_HALF + (offsetX + (mirror ? -p[0] : p[0])) * t,
				  p[1] * t,
				  -t + p[2] * t,
				  &light,
				  &fire,
				  seed + static_cast<uint32_t>(n)};
		n++;
	};

	const DecorCell& cell = decor[MapIndex(i, j)];
	if (cell.type == DECOR_BRAZIER)
		at(BRAZIER_FIRE, cell.offsetX, cell.mirror, Lighting::BRAZIER, Fire::BRAZIER);
	else if (cell.type == DECOR_LAMP)
		at(LAMP_FIRE, cell.offsetX, cell.mirror, Lighting::OIL_LAMP, Fire::OIL_LAMP);
	if (torch[MapIndex(i, j)])
		at(TORCH_FIRE, 0.f, false, Lighting::TORCH, Fire::TORCH);
	return n;
}
//======================================================================================
// Lights from flames a little beyond the drawn window too, so light spills in before its source is visible.
// Same frame as the tile loop in Draw(): cell (col0, row0) of the window sits at the origin.
void Dungeon::addLights(const CellRect& drawn) {
	const int col0 = view().originCol;
	const int row0 = view().originRow;
	const CellRect lit = drawn.grown(2);
	for (int j = lit.row0; j < lit.row0 + lit.rows; j++)
		for (int i = lit.col0; i < lit.col0 + lit.cols; i++) {
			if (!IsInBounds(i, j))
				continue;
			FlameSource flames[2];
			int n = flamesAt(i, j, flames);
			float ox = static_cast<float>(i - col0) * RenderConfig::TILE_SIZE;
			float oy = static_cast<float>(j - row0) * RenderConfig::TILE_SIZE;
			for (int k = 0; k < n; k++)
				Lighting::add(ox + flames[k].x, oy + flames[k].y + LIGHT_LIFT, flames[k].z + LIGHT_LIFT,
							  *flames[k].light, flames[k].seed);
		}
}
//======================================================================================
// After all opaque tiles: the sprites do not write depth, so later tiles would paint over them.
void Dungeon::drawFires(const CellRect& drawn) {
	const int col0 = view().originCol;
	const int row0 = view().originRow;
	for (int j = drawn.row0; j < drawn.row0 + drawn.rows; j++)
		for (int i = drawn.col0; i < drawn.col0 + drawn.cols; i++) {
			if (!IsInBounds(i, j))
				continue;
			FlameSource flames[2];
			int n = flamesAt(i, j, flames);
			float ox = static_cast<float>(i - col0) * RenderConfig::TILE_SIZE;
			float oy = static_cast<float>(j - row0) * RenderConfig::TILE_SIZE;
			for (int k = 0; k < n; k++)
				Fire::draw(*flames[k].fire, ox + flames[k].x, oy + flames[k].y, flames[k].z, flames[k].seed);
		}
}
//======================================================================================
// Each row of open cells between two solid ones is rough rock (more often deeper down) or dressed stone. Dressed
// rows are cut into stretches, each painted or bare; a painted stretch ends in broken plaster where bare stone
// follows (stretches are at least STRETCH_MIN long, so only the one clipped by the row's end can be shorter, and it has
// stone on one side at most). Then a variant per cell, never the same uncommon one twice in a row.
void Dungeon::scatterSurfaces(uint32_t seed) {
	enum Family : uint8_t { Painted, Stone, Rough };
	uint32_t level = static_cast<uint32_t>(std::max(levelNumber, 1)) - 1;
	uint32_t roughPercent = std::min(ROUGH_PERCENT_FIRST + ROUGH_PERCENT_STEP * level, ROUGH_PERCENT_MAX);
	Family family[MAP_WIDTH];
	int cells[3] = {};

	for (int j = 0; j < MAP_HEIGHT; j++)
		for (int start = 0; start < MAP_WIDTH;) {
			if (isWall(MapAt(start, j))) {
				start++;
				continue;
			}
			int end = start;
			while (end < MAP_WIDTH && !isWall(MapAt(end, j)))
				end++;

			uint32_t h = cellHash(seed, MapIndex(start, j));
			bool rough = h % 100 < roughPercent;
			for (int i = start; i < end;) {
				h = mix(h);
				int len = STRETCH_MIN + static_cast<int>(h % STRETCH_SPAN);
				h = mix(h);
				Family f = rough ? Rough : ((h >> 8) % 100 < PAINTED_PERCENT ? Painted : Stone);
				for (int k = i; k < std::min(i + len, end); k++)
					family[k] = f;
				i += len;
			}

			int prevWall = -1;
			for (int i = start; i < end; i++) {
				SurfaceCell& cell = surface[MapIndex(i, j)];
				h = mix(h ^ static_cast<uint32_t>(i));
				uint32_t roll = h % 100;
				cell.wallMirror = false; // a mirrored neighbour would show as a mirror line at the seam
				switch (family[i]) {
				case Painted:
					cell.ceiling = CEILING_STARS;
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
				bool common = cell.wall == WALL_PLASTER || cell.wall == WALL_STONE || cell.wall == WALL_ROUGH;
				if (!common && cell.wall == prevWall && cell.wall != WALL_PLASTER_BROKEN)
					cell.wall = family[i] == Painted ? WALL_PLASTER : (family[i] == Stone ? WALL_STONE : WALL_ROUGH);
				prevWall = cell.wall;
				cells[family[i]]++;

				h = mix(h);
				roll = h % 100;
				uint32_t sandPercent = family[i] == Rough ? 60 : 30;
				cell.floor = roll < sandPercent ? FLOOR_SAND : (roll < sandPercent + 20 ? FLOOR_CRACKED : FLOOR_SLABS);
			}
			start = end;
		}
	LOG_INFOF("world", "Surfaces: %d painted, %d stone, %d rough cells", cells[Painted], cells[Stone], cells[Rough]);
}
