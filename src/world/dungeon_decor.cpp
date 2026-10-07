#include "dungeon.h"
#include "../state/assets.h"
#include "../graphics/render_config.h"
#include "../graphics/fire.h"
#include "../graphics/lighting.h"
#include "../core/logger.h"
#include <GL/gl.h>
#include <utility>

namespace {
// Flame origins in prop space (tile units, x before mirroring), from the geometry in decor.py.
constexpr float BRAZIER_FIRE[3] = {0.f, 0.22f, 0.16f};	 // on the charcoal
constexpr float LAMP_FIRE[3] = {-0.256f, 0.05f, 0.307f}; // oil lamp wick
constexpr float TORCH_FIRE[3] = {0.f, 0.68f, 0.098f};	 // top of the torch head
constexpr float LIGHT_LIFT = 4.f;						 // lights sit above and in front of the flame (world units)
} // namespace

CoffinBoss Dungeon::coffinBoss() const {
	const Assets* assets = sim.assets;
	return [assets](int type) { return assets->monsterTypes[type].boss.summon == Summon::Coffin; };
}

bool Dungeon::bossCoffin(int i, int j) const { return bossCoffinCell(map, i, j, coffinBoss()); }

int Dungeon::CoffinCount() const {
	int n = 0;
	for (const DecorCell& cell : decoration.decor)
		if (cell.type == DECOR_COFFIN)
			n++;
	return n;
}

bool Dungeon::PlaceDecor(int col, int row, int type) {
	if (!IsInBounds(col, row) || type < 0 || type >= DECOR_COUNT)
		return false;
	decoration.decor[MapIndex(col, row)] = DecorCell{static_cast<int8_t>(type), false, 0.f};
	return true;
}

int Dungeon::DecorTierUsed() const { return decorTierUsed(map, decoration); }

void Dungeon::scatterDecorations(const char* levelName, int depth) {
	const DecorCounts counts = scatterDecor(map, levelName, depth, coffinBoss(), decoration);
	LOG_INFOF("world", "Decorations in %s: %d", levelName, counts.props);
	LOG_INFOF("world", "Torches: %d", counts.torches);
	LOG_INFOF("world", "Wall decals: %d", counts.decals);
	LOG_INFOF("world", "Ladder shafts: %d", counts.ladderShafts);
	LOG_INFOF("world", "Surfaces: %d painted, %d stone, %d rough cells", counts.painted, counts.stone, counts.rough);
}
//======================================================================================
void Dungeon::drawDecorTile(int i, int j) {
	const DecorCell& cell = decoration.decor[MapIndex(i, j)];
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
	const DecalCell& cell = decoration.decal[MapIndex(i, j)];
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
	if (!decoration.torch[MapIndex(i, j)] || model == nullptr)
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
	const LadderCell& cell = decoration.ladder[MapIndex(i, j)];
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

	const DecorCell& cell = decoration.decor[MapIndex(i, j)];
	if (cell.type == DECOR_BRAZIER)
		at(BRAZIER_FIRE, cell.offsetX, cell.mirror, Lighting::BRAZIER, Fire::BRAZIER);
	else if (cell.type == DECOR_LAMP)
		at(LAMP_FIRE, cell.offsetX, cell.mirror, Lighting::OIL_LAMP, Fire::OIL_LAMP);
	if (decoration.torch[MapIndex(i, j)])
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
