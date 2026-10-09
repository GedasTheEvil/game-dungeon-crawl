// Drawing the decorations scatterDecor placed (decor_scatter.h): props, decals, torches, ladders, and the flames and
// lights of the fires among them.
#include "dungeon.h"
#include "../state/assets.h"
#include "../graphics/render_config.h"
#include "../graphics/fire.h"
#include "../graphics/lighting.h"
#include <GL/gl.h>
#include <utility>

namespace {
constexpr float LIGHT_LIFT = 4.f; // lights sit above and in front of the flame (world units)
} // namespace

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
namespace {
// A flame's spot in the frame drawDecorTile starts from (world units), its light and its fire sprite.
struct FlameSource {
	float x, y, z;
	const Lighting::LightDef* light;
	const FireStyle* fire;
	uint32_t seed;
};

FlameSource flameSource(const Flame& f) {
	const float t = RenderConfig::TILE_SIZE;
	FlameSource out{RenderConfig::TILE_HALF + f.x * t, f.y * t, -t + f.z * t, &Lighting::TORCH, &Fire::TORCH, f.seed};
	if (f.kind == FlameKind::Brazier) {
		out.light = &Lighting::BRAZIER;
		out.fire = &Fire::BRAZIER;
	} else if (f.kind == FlameKind::OilLamp) {
		out.light = &Lighting::OIL_LAMP;
		out.fire = &Fire::OIL_LAMP;
	}
	return out;
}
} // namespace

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
			Flame flames[MAX_CELL_FLAMES];
			int n = flamesAt(decoration, MapIndex(i, j), flames);
			float ox = static_cast<float>(i - col0) * RenderConfig::TILE_SIZE;
			float oy = static_cast<float>(j - row0) * RenderConfig::TILE_SIZE;
			for (int k = 0; k < n; k++) {
				const FlameSource f = flameSource(flames[k]);
				Lighting::add(ox + f.x, oy + f.y + LIGHT_LIFT, f.z + LIGHT_LIFT, *f.light, f.seed);
			}
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
			Flame flames[MAX_CELL_FLAMES];
			int n = flamesAt(decoration, MapIndex(i, j), flames);
			float ox = static_cast<float>(i - col0) * RenderConfig::TILE_SIZE;
			float oy = static_cast<float>(j - row0) * RenderConfig::TILE_SIZE;
			for (int k = 0; k < n; k++) {
				const FlameSource f = flameSource(flames[k]);
				Fire::draw(*f.fire, ox + f.x, oy + f.y, f.z, f.seed);
			}
		}
}
