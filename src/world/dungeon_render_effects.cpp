// Drawing the monsters, the missiles and venom in flight, and the boss fight's summon effects (the rules are in
// dungeon_monsters.cpp, dungeon_arrows.cpp and dungeon_boss.cpp).
#include "dungeon.h"
#include "dungeon_rules.h"
#include "../state/assets.h"
#include "../entities/monster_draw.h"
#include "../entities/player.h"
#include "../graphics/fire.h"
#include "../graphics/ink.h"
#include "../graphics/lighting.h"
#include "../graphics/render_config.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>

namespace {
// The held bow is drawn 12 units tall for its 1.2 m (ITEMS in world/items.cpp, items.py); arrow.md3 is in metres.
constexpr float ARROW_WORLD_PER_METRE = 10.f;
constexpr float ARROW_DEPTH = -18.f; // just in front of the monsters (-20)
constexpr float RAD_TO_DEG = 57.29578f;
constexpr float PI = 3.14159265f;
} // namespace

// Called in Draw() with the frame origin at the view's first column (ViewWindow::firstCol).
// A monster beyond the gameplay window but in the drawn one (a wide window) is drawn too.
void Dungeon::DrawMonsters(const CellRect& drawn) {
	const int firstCol = view().firstCol();
	const int firstRow = view().originRow;
	for (Monster& mon : monsters) {
		if (!mon.Active() || !drawn.contains(mon.CentreX(), mon.Row()))
			continue;

		glPushMatrix();
		glTranslatef(RenderConfig::TILE_SIZE * static_cast<float>(mon.Col() - firstCol),
					 RenderConfig::TILE_SIZE * static_cast<float>(mon.Row() - firstRow), 0);
		glTranslatef(RenderConfig::MONSTER_OFFSET_X, 0, RenderConfig::MONSTER_OFFSET_Z);
		DrawMonster(mon, sim.assets->monsterModels[mon.Type()->id], sim.assets->textures);
		glPopMatrix();
	}
}
//======================================================================================
void Dungeon::drawMissiles() {
	const auto firstCol = static_cast<float>(view().firstCol()); // DrawMonsters' frame
	const auto firstRow = static_cast<float>(view().originRow);
	const int now = GameClock::now();
	for (const Missile& a : missiles) {
		AnimatedModel* model = sim.assets->items.missiles[static_cast<size_t>(a.kind)].get();
		if (!model)
			continue;
		const MissileRules& rules = rulesOf(a.kind);
		const float scale = ARROW_WORLD_PER_METRE * Ink::figureScale() * rules.scale;
		const auto [low, high] = model->YRange(0);
		float x = a.X(), y = a.Y();
		if (rules.returns && a.stuckMs >= 0) { // back to the player's hand
			const float k = std::min(1.f, static_cast<float>(now - a.stuckMs) / static_cast<float>(STICK_RETURN_MS));
			const float handY = mapY + 0.5f * sim.player->Height();
			x = a.endX + (mapX - a.endX) * k;
			y = a.endY + (handY - a.endY) * k;
		}
		glPushMatrix();
		glTranslatef(RenderConfig::TILE_SIZE * (x - firstCol), RenderConfig::TILE_SIZE * (y - firstRow), ARROW_DEPTH);
		if (rules.spinDegPerS > 0.f) { // round its middle
			const float turned = rules.spinDegPerS * static_cast<float>(now - a.startMs) / 1000.f;
			glRotatef(a.vx > 0.f ? -turned : turned, 0, 0, 1);
			glTranslatef(0, -(low + high) / 2.f * scale, 0);
		} else { // the model points up (+y), the tip at the flight position
			const float heading = std::atan2(a.vy - ARROW_GRAVITY * a.t, a.vx) * RAD_TO_DEG;
			glRotatef(heading - 90.f, 0, 0, 1);
			glTranslatef(0, -high * scale, 0);
		}
		glScalef(scale, scale, scale);
		model->Show();
		glPopMatrix();
	}
}
//======================================================================================
// The bow's arrow, smaller, pointing at the camera, from the back wall (-TILE_SIZE) out to the front (0).
void Dungeon::drawDarts() {
	AnimatedModel* model = sim.assets->items.missiles[static_cast<size_t>(MissileKind::Arrow)].get();
	if (darts.empty() || !model)
		return;
	const auto firstCol = static_cast<float>(view().firstCol()); // DrawMonsters' frame
	const auto firstRow = static_cast<float>(view().originRow);
	constexpr float DART_SCALE = 0.8f; // of the arrow model
	const float scale = ARROW_WORLD_PER_METRE * Ink::figureScale() * DART_SCALE;
	const float high = model->YRange(0).second;
	for (const Dart& d : darts) {
		glPushMatrix();
		glTranslatef(RenderConfig::TILE_SIZE * (d.x - firstCol), RenderConfig::TILE_SIZE * (d.y - firstRow),
					 -RenderConfig::TILE_SIZE * (1.f - d.depth));
		glRotatef(90.f, 1, 0, 0); // the model points up (+y): now at the camera (+z), the tip at the position
		glTranslatef(0, -high * scale, 0);
		glScalef(scale, scale, scale);
		model->Show();
		glPopMatrix();
	}
}
//======================================================================================
// A glob of venom (a bolt of light): a small ball in its spitter's colour, stretched along its flight; on a wall or the
// floor a flattening splat.
void Dungeon::drawVenoms() {
	if (venoms.empty())
		return;
	constexpr float RADIUS = 1.6f; // world units
	constexpr int SLICES = 8, STACKS = 6;
	const auto firstCol = static_cast<float>(view().firstCol()); // DrawMonsters' frame
	const auto firstRow = static_cast<float>(view().originRow);
	const int now = GameClock::now();
	sim.assets->textures.nullTex.Bind(); // texture x colour: plain colour
	glEnable(GL_BLEND);
	Lighting::setEmissive(true);
	for (const Venom& v : venoms) {
		float stretch = 1.6f, squash = 1.f, alpha = 0.85f;
		if (v.splatMs >= 0) {
			const float k = std::min(static_cast<float>(now - v.splatMs) / static_cast<float>(VENOM_SPLAT_MS), 1.f);
			stretch = 1.f + 1.5f * k;
			squash = 1.f - 0.7f * k;
			alpha *= 1.f - k;
		}
		glPushMatrix();
		glTranslatef(RenderConfig::TILE_SIZE * (v.x - firstCol), RenderConfig::TILE_SIZE * (v.y - firstRow),
					 ARROW_DEPTH);
		glRotatef(std::atan2(v.vy, v.vx) * RAD_TO_DEG, 0, 0, 1);
		glScalef(RADIUS * stretch, RADIUS * squash, RADIUS);
		glColor4f(v.colour.r, v.colour.g, v.colour.b, alpha);
		for (int i = 0; i < STACKS; i++) {
			const float a0 = PI * static_cast<float>(i) / STACKS - PI / 2, a1 = a0 + PI / STACKS;
			glBegin(GL_TRIANGLE_STRIP);
			for (int j = 0; j <= SLICES; j++) {
				const float b = 2 * PI * static_cast<float>(j) / SLICES;
				for (float a : {a0, a1}) {
					const float nx = std::cos(a) * std::cos(b), ny = std::cos(a) * std::sin(b), nz = std::sin(a);
					glNormal3f(nz, ny, nx);
					glVertex3f(nz, ny, nx); // the poles along x, the flight
				}
			}
			glEnd();
		}
		glPopMatrix();
	}
	Lighting::setEmissive(false);
	glDisable(GL_BLEND);
	glColor3f(1, 1, 1);
}
//======================================================================================
// Where a minion came out, while its summon lasts: sand thrown up from the floor or grit falling from the ceiling.
// Same window and frame as drawMechanismEffects.
void Dungeon::drawSummonEffects() {
	const int now = GameClock::now();
	for (const Monster& mon : monsters) {
		const int age = now - mon.SummonedMs();
		if (!mon.Active() || mon.SummonedMs() < 0 || mon.SummonedBy() == Summon::Coffin || age >= Grit::BURST_MS)
			continue;
		const float x = (mon.CentreX() - static_cast<float>(view().originCol)) * RenderConfig::TILE_SIZE;
		const bool drop = mon.SummonedBy() == Summon::Drop;
		const float y = static_cast<float>(mon.Row() - view().originRow + (drop ? 1 : 0)) * RenderConfig::TILE_SIZE;
		Grit::burst(x, y, RenderConfig::MONSTER_DEPTH, age, drop, static_cast<uint32_t>(mon.SummonedMs() + mon.Col()));
	}
	holes.erase(
		std::remove_if(holes.begin(), holes.end(), [now](const Hole& h) { return now - h.startMs >= BURROW_HOLE_MS; }),
		holes.end());
	for (const Hole& h : holes) {
		const float x = (h.x - static_cast<float>(view().originCol)) * RenderConfig::TILE_SIZE;
		const float y = static_cast<float>(h.row - view().originRow) * RenderConfig::TILE_SIZE;
		const int age = now - h.startMs;
		if (age < Grit::BURST_MS)
			Grit::burst(x, y, RenderConfig::MONSTER_DEPTH, age, false,
						static_cast<uint32_t>(h.startMs) + static_cast<uint32_t>(h.row));
		drawHole(x, y + 0.3f, RenderConfig::MONSTER_DEPTH,
				 1.f - static_cast<float>(age) / static_cast<float>(BURROW_HOLE_MS));
	}
}
//======================================================================================
// A dark pit in the floor, fading out (fade 1..0): a flat disc round (x, y, z), wider along the row.
void Dungeon::drawHole(float x, float y, float z, float fade) const {
	constexpr float RX = 16.f, RZ = 7.f; // world units
	constexpr int SIDES = 20;
	sim.assets->textures.nullTex.Bind();
	Lighting::setEmissive(true);
	glEnable(GL_BLEND);
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(0.02f, 0.01f, 0.f, 0.9f * fade);
	glVertex3f(x, y, z);
	glColor4f(0.1f, 0.07f, 0.03f, 0.f);
	for (int i = 0; i <= SIDES; i++) {
		const float a = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / SIDES;
		glVertex3f(x + RX * std::cos(a), y, z + RZ * std::sin(a));
	}
	glEnd();
	glDisable(GL_BLEND);
	Lighting::setEmissive(false);
	glColor3f(1, 1, 1);
}
