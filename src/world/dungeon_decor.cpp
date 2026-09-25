#include "dungeon.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../graphics/render_config.h"
#include "../core/logger.h"
#include <GL/gl.h>
#include <cstdint>
#include <algorithm>
#include <cstring>

namespace {
constexpr uint32_t DECOR_CHANCE_PERCENT = 30;
constexpr uint32_t DECAL_CHANCE_PERCENT = 35;
constexpr uint32_t DECAL_SALT = 0x51ed270bU;
// Horizontal jitter per prop (tile units), from the extents decor.py prints, so props stay inside the tile.
constexpr float DECOR_JITTER[DECOR_COUNT] = {0.f, 0.12f, 0.06f, 0.1f, 0.f, 0.15f, 0.2f, 0.1f, 0.1f, 0.06f};

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
} // namespace

// Every cell rolls independently from (level name, cell index), so the layout is the same on every
// load of a level and does not depend on the rest of the map.
void Dungeon::scatterDecorations(const char* levelName) {
	const char* slash = strrchr(levelName, '/');
	uint32_t seed = hashName(slash != nullptr ? slash + 1 : levelName);
	int placed = 0;

	for (int j = 0; j < kMapHeight; j++)
		for (int i = 0; i < kMapWidth; i++) {
			DecorCell& cell = decor[MapIndex(i, j)];
			cell = DecorCell{};

			// Only empty cells the player can stand in (floor below).
			if (MapAt(i, j).a != Empty || !IsInBounds(i, j - 1) || MapAt(i, j - 1).a != Wall)
				continue;

			uint32_t h = mix(seed ^ mix(static_cast<uint32_t>(MapIndex(i, j)) + 0x9e3779b9U));
			if (h % 100 >= DECOR_CHANCE_PERCENT)
				continue;

			bool wallLeft = !IsInBounds(i - 1, j) || MapAt(i - 1, j).a == Wall;
			bool wallRight = !IsInBounds(i + 1, j) || MapAt(i + 1, j).a == Wall;
			bool ceiling = !IsInBounds(i, j + 1) || MapAt(i, j + 1).a == Wall;
			bool webFits = ceiling; // lies flat on the back wall, tucked into a side wall's corner if there is one

			h = mix(h);
			int type = webFits ? static_cast<int>(h % DECOR_COUNT) : 1 + static_cast<int>(h % (DECOR_COUNT - 1));
			h = mix(h);
			cell.type = static_cast<int8_t>(type);
			if (type == DECOR_WEB)
				cell.mirror = wallLeft == wallRight ? (h & 1U) != 0 : wallRight;
			else
				cell.mirror = (h & 1U) != 0;
			cell.offsetX = (unit01(h) * 2.f - 1.f) * DECOR_JITTER[type];
			placed++;
		}
	LOG_INFOF("world", "Decorations in %s: %d", levelName, placed);

	scatterDecals(seed ^ DECAL_SALT);
}
//======================================================================================
// One decal at most per cell with a visible back wall. Runs after the props so floor decals can
// avoid cells that already have a prop.
void Dungeon::scatterDecals(uint32_t seed) {
	int placed = 0;

	for (int j = 0; j < kMapHeight; j++)
		for (int i = 0; i < kMapWidth; i++) {
			DecalCell& cell = decal[MapIndex(i, j)];
			cell = DecalCell{};

			int tile = MapAt(i, j).a;
			if (tile == Wall || tile == Door || tile == Ladder)
				continue;

			uint32_t h = mix(seed ^ mix(static_cast<uint32_t>(MapIndex(i, j)) + 0x9e3779b9U));
			if (h % 100 >= DECAL_CHANCE_PERCENT)
				continue;

			bool ceiling = !IsInBounds(i, j + 1) || MapAt(i, j + 1).a == Wall;
			bool floor = IsInBounds(i, j - 1) && MapAt(i, j - 1).a == Wall && decor[MapIndex(i, j)].type < 0;

			int fits[DECAL_COUNT];
			int fitCount = 0;
			for (int d = 0; d < DECAL_COUNT; d++) {
				DecalAnchor anchor = DECAL_DEFS[d].anchor;
				if (anchor == DecalAnchor::Free || (anchor == DecalAnchor::Ceiling && ceiling) ||
					(anchor == DecalAnchor::Floor && floor))
					fits[fitCount++] = d;
			}

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

	AnimatedCartoonModel* model = GAME_STATE.decor.model[cell.type].get();
	if (model == nullptr)
		return;

	glPushMatrix();
	glTranslatef(RenderConfig::TILE_HALF + cell.offsetX * RenderConfig::TILE_SIZE, 0, -RenderConfig::TILE_SIZE);
	glScalef(cell.mirror ? -RenderConfig::TILE_SIZE : RenderConfig::TILE_SIZE, RenderConfig::TILE_SIZE,
			 RenderConfig::TILE_SIZE);

	// Textured only (lighting is baked in); the toon pass would wash out dark details.
	GAME_STATE.decor.tex[cell.type].Bind();
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
	GAME_STATE.decor.decalTex.Bind();
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
