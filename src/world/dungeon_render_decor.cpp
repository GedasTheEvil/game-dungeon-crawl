// Drawing the decorations scatterDecor placed (decor_scatter.h): props, decals, torches, ladders, and the flames and
// lights of the fires among them. Also the inscriptions, decals the level places.
#include "dungeon.h"
#include "names.h"
#include "../state/assets.h"
#include "../graphics/render_config.h"
#include "../graphics/fire.h"
#include "../graphics/lighting.h"
#include "inscriptions.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace {
constexpr float LIGHT_LIFT = 4.f; // lights sit above and in front of the flame (world units)
constexpr float INSCRIPTION_SIZE = 0.7f; // quad side, tile units, a little larger than the scattered glyph decals
constexpr float INSCRIPTION_Y = 0.55f; // its centre on the back wall: at the archaeologist's eye height
// The glint over an unread inscription (plan interact-prompt): a gold band sweeps across the glyphs, then rests.
constexpr int GLINT_PERIOD_MS = 3200;
constexpr float GLINT_SWEEP = 0.4f; // share of the period the band takes to cross
constexpr float GLINT_WIDTH = 0.3f; // the band's half width, in shares of the quad
constexpr float GLINT_SLANT = 0.35f; // the band leans: its top runs this far ahead
constexpr int GLINT_STRIPS = 12;
constexpr int GLINT_PASSES = 2; // added this many times: one is too faint on the torchlit stele
constexpr float GLINT_COLOUR[3] = {1.f, 0.82f, 0.45f};
// Water props (plan water-decorations), tile units and radians a second. The reeds and the weed sway (x sheared by
// the height over the basin floor), the leaves afloat bob and drift.
constexpr float BASIN_FLOOR = -RenderConfig::WATER_BASIN_DEPTH / RenderConfig::TILE_SIZE;
constexpr float REED_SWAY = 0.04f; // the shear: 0.03 tile at the reeds' tips
constexpr float REED_SWAY_SPEED = 0.9f;
constexpr float WEED_SWAY = 0.12f; // in the current
constexpr float WEED_SWAY_SPEED = 0.6f;
constexpr float BOB_HEIGHT = 0.005f;
constexpr float BOB_SPEED = 1.4f;
constexpr float DRIFT_WIDTH = 0.012f;
constexpr float DRIFT_SPEED = 0.35f;

// A square of side `size` centred on (x, y) of the cell's back wall (tile units), textured from the bound atlas,
// blended over the wall.
void wallQuad(float x, float y, float size, AtlasRect uv, bool mirror) {
	const float tile = RenderConfig::TILE_SIZE;
	const float half = size * tile / 2.f;
	const float cx = x * tile;
	const float cy = y * tile;
	const float z = -tile + 0.05f; // just in front of the back wall
	if (mirror)
		std::swap(uv.u0, uv.u1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glTexCoord2f(uv.u0, uv.v0);
	glVertex3f(cx - half, cy - half, z);
	glTexCoord2f(uv.u1, uv.v0);
	glVertex3f(cx + half, cy - half, z);
	glTexCoord2f(uv.u1, uv.v1);
	glVertex3f(cx + half, cy + half, z);
	glTexCoord2f(uv.u0, uv.v1);
	glVertex3f(cx - half, cy + half, z);
	glEnd();
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

// The glint over wallQuad(x, y, size, uv): the bound atlas again, added in gold through a band whose place follows the
// clock, each cell on its own phase. The texture weighs it (colour and alpha): the light stele catches it most.
void glintQuad(float x, float y, float size, AtlasRect uv, float strength, int cell) {
	const int offset = static_cast<int>((static_cast<uint32_t>(cell) * 2654435761U) >> 20) % GLINT_PERIOD_MS;
	const float t = static_cast<float>((GameClock::now() + offset) % GLINT_PERIOD_MS) /
					static_cast<float>(GLINT_PERIOD_MS) / GLINT_SWEEP;
	if (t >= 1.f)
		return; // resting
	const float centre = -GLINT_WIDTH - GLINT_SLANT + t * (1.f + 2 * GLINT_WIDTH + GLINT_SLANT);
	const auto band = [&](float u, float v) {
		return strength * std::max(0.f, 1.f - std::fabs(u + GLINT_SLANT * v - centre) / GLINT_WIDTH);
	};
	const float tile = RenderConfig::TILE_SIZE;
	const float half = size * tile / 2.f;
	const float z = -tile + 0.06f; // over the inscription

	Lighting::setEmissive(true);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDepthMask(GL_FALSE);
	for (int pass = 0; pass < GLINT_PASSES; pass++) {
		glBegin(GL_QUAD_STRIP);
		glNormal3f(0, 0, 1);
		for (int i = 0; i <= GLINT_STRIPS; i++) {
			const float u = static_cast<float>(i) / GLINT_STRIPS;
			const float px = x * tile - half + u * 2 * half;
			const float tu = uv.u0 + u * (uv.u1 - uv.u0);
			glColor4f(GLINT_COLOUR[0], GLINT_COLOUR[1], GLINT_COLOUR[2], band(u, 0.f));
			glTexCoord2f(tu, uv.v0);
			glVertex3f(px, y * tile - half, z);
			glColor4f(GLINT_COLOUR[0], GLINT_COLOUR[1], GLINT_COLOUR[2], band(u, 1.f));
			glTexCoord2f(tu, uv.v1);
			glVertex3f(px, y * tile + half, z);
		}
		glEnd();
	}
	glDepthMask(GL_TRUE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glColor3f(1, 1, 1);
	Lighting::setEmissive(false);
}
} // namespace

namespace {
// The water prop's sway or bob, in its model space; each cell on its own phase.
void waterMotion(int type, int cell) {
	const float time = static_cast<float>(GameClock::now()) / 1000.f;
	const float phase = static_cast<float>((static_cast<uint32_t>(cell) * 2654435761U) >> 20) / 4096.f * 6.2832f;
	if (type == DECOR_WATER_REEDS || type == DECOR_WATER_WEED) {
		const bool reeds = type == DECOR_WATER_REEDS;
		const float k =
			(reeds ? REED_SWAY : WEED_SWAY) * std::sin(time * (reeds ? REED_SWAY_SPEED : WEED_SWAY_SPEED) + phase);
		// x += k * (y - BASIN_FLOOR): the foot stays put, the tips move.
		const float shear[16] = {1, 0, 0, 0, k, 1, 0, 0, 0, 0, 1, 0, -k * BASIN_FLOOR, 0, 0, 1};
		glMultMatrixf(shear);
	} else
		glTranslatef(DRIFT_WIDTH * std::sin(time * DRIFT_SPEED + phase),
					 BOB_HEIGHT * std::sin(time * BOB_SPEED + phase), 0);
}
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
	if (isWaterProp(cell.type))
		waterMotion(cell.type, MapIndex(i, j));

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
	sim.assets->decor.decalTex.Bind();
	wallQuad(cell.x, cell.y, DECAL_DEFS[cell.type].size, atlasCell(cell.type, DECAL_ATLAS_GRID), cell.mirror);
}
//======================================================================================
// Without its own glyphs (an id outside the atlas) it shows the scattered cartouche.
void Dungeon::drawInscriptionTile(int i, int j) {
	const int id = MapAt(i, j).fixture.attr;
	AtlasRect uv =
		isInscriptionId(id) ? atlasCell(id - 1, INSCRIPTION_ATLAS_GRID) : atlasCell(DECAL_CARTOUCHE, DECAL_ATLAS_GRID);
	if (id == NAME_CARTOUCHE_ID) { // plan secret-name-gates: this run's picture for the level
		uv = atlasCell(NameCartouchePicture(), NAME_CARTOUCHE_GRID);
		sim.assets->decor.nameCartoucheTex.Bind();
	} else
		(isInscriptionId(id) ? sim.assets->decor.inscriptionTex : sim.assets->decor.decalTex).Bind();
	wallQuad(0.5f, INSCRIPTION_Y, INSCRIPTION_SIZE, uv, false);
	if (const float glint = InscriptionGlint(i, j); glint > 0.f)
		glintQuad(0.5f, INSCRIPTION_Y, INSCRIPTION_SIZE, uv, glint, MapIndex(i, j));
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
