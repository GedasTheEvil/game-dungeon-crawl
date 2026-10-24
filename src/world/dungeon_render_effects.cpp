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
constexpr float GLOB_RADIUS = 1.2f; // world units
constexpr float GLOB_LIGHT_LIFT = 1.f;
// Faint, steady and green: the glob's own light on the walls it passes.
constexpr Lighting::LightDef VENOM_LIGHT = {0.45f, 0.75f, 0.15f, 40.f, 0.f};
// A fire arrow's and a burning body's flicker (plan fire-damage).
constexpr Lighting::LightDef FIRE_ARROW_LIGHT = {1.f, 0.6f, 0.25f, 45.f, 0.5f};
constexpr Lighting::LightDef BURNING_LIGHT = {1.f, 0.55f, 0.2f, 80.f, 0.7f};
constexpr float BURNING_FRONT = 3.f; // world units in front of the body's centre: the flames lick round its front
constexpr float SPRAY_SPREAD = 0.3f; // tiles between the bursts of a sand spray

// A fiery bolt's flickering light, an oil lamp's (plan anubis-fire-bolt).
constexpr Lighting::LightDef FIRE_BOLT_LIGHT = {1.f, 0.6f, 0.25f, 60.f, 0.5f};
constexpr float FIRE_BOLT_CORE = 0.6f; // the fireball's glowing core, of the plain glob's size

// Flame clouds trailing behind a fiery glob, in world units along its flight.
constexpr float GLOB_TRAIL_STEP = 2.2f;
constexpr int GLOB_TRAIL_CLOUDS = 2;

// A ball in `colour` of `radius` world units, stretched along its flight (vx, vy); `plain` is the white texture.
void drawBall(
	Texture& plain,
	Rgb colour,
	float x,
	float y,
	float z,
	float vx,
	float vy,
	float stretch,
	float squash,
	float alpha,
	float radius
) {
	constexpr int SLICES = 8, STACKS = 6;
	plain.Bind(); // texture x colour: plain colour
	glEnable(GL_BLEND);
	Lighting::setEmissive(true);
	glPushMatrix();
	glTranslatef(x, y, z);
	glRotatef(std::atan2(vy, vx) * RAD_TO_DEG, 0, 0, 1);
	glScalef(radius * stretch, radius * squash, radius);
	glColor4f(colour.r, colour.g, colour.b, alpha);
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
	Lighting::setEmissive(false);
	glDisable(GL_BLEND);
	glColor3f(1, 1, 1);
}

// How a shot looks: a plain glob of light, or flames (the style carries the colours) with smaller clouds trailing in
// flight and a burst on a splat; with a core, a small glowing ball inside the flames.
struct GlobLook {
	const FireStyle* flames = nullptr; // null: the plain glob
	const FireStyle* trail = nullptr;
	const FireStyle* splat = nullptr;
	const Lighting::LightDef* light = nullptr; // in flight
	bool core = false;
};
constexpr GlobLook PLAIN_GLOB{};
constexpr GlobLook VENOM_GLOB =
	{&Fire::VENOM, &Fire::VENOM_TRAIL, &Fire::VENOM, &VENOM_LIGHT, false}; // all flames, no ball
constexpr GlobLook FIRE_BOLT = {&Fire::BOLT, &Fire::BOLT_TRAIL, &Fire::BOLT_SPLAT, &FIRE_BOLT_LIGHT, true};

const GlobLook& globLook(const std::optional<RangedRules>& ranged) {
	if (!ranged.has_value())
		return PLAIN_GLOB;
	if (ranged->poison.has_value())
		return VENOM_GLOB;
	return ranged->burns() ? FIRE_BOLT : PLAIN_GLOB;
}

// A glob of light or of flames, at (x, y, z) in the frame origin of its tile, flying along (vx, vy), in its look.
// Shared by the venom and the Anubis fire bolt.
void drawGlobWithFlames(
	Texture& plain,
	Rgb colour,
	const GlobLook& look,
	uint32_t seed,
	float x,
	float y,
	float z,
	float vx,
	float vy,
	float stretch,
	float squash,
	float alpha
) {
	const bool splat = squash < 1.f;
	if (look.flames == nullptr || (look.core && !splat))
		drawBall(
			plain,
			colour,
			x,
			y,
			z,
			vx,
			vy,
			stretch,
			squash,
			alpha,
			look.flames == nullptr ? GLOB_RADIUS : GLOB_RADIUS * FIRE_BOLT_CORE
		);
	if (look.flames == nullptr)
		return;
	if (splat) { // a burst fading with the splat
		Fire::draw(*look.splat, x, y, z, seed, alpha);
		return;
	}
	Fire::draw(*look.flames, x, y, z, seed); // billboards: no rotation in the modelview
	const float speed = std::hypot(vx, vy);
	if (speed <= 0.f)
		return;
	for (int i = 1; i <= GLOB_TRAIL_CLOUDS; i++) {
		const float back = GLOB_TRAIL_STEP * static_cast<float>(i) / speed;
		Fire::draw(*look.trail, x - vx * back, y - vy * back, z, seed + static_cast<uint32_t>(i) * 7919u);
	}
}
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
		glTranslatef(
			RenderConfig::TILE_SIZE * static_cast<float>(mon.Col() - firstCol),
			RenderConfig::TILE_SIZE * static_cast<float>(mon.Row() - firstRow),
			0
		);
		glTranslatef(RenderConfig::MONSTER_OFFSET_X, 0, RenderConfig::MONSTER_OFFSET_Z);
		DrawMonster(mon, sim.assets->monsterModels[mon.Type()->id], sim.assets->textures);
		glPopMatrix();
	}
	drawDecoys(drawn);
}
//======================================================================================
// The decoys (plan placed-decor): a mummy asleep in every DECOR_COFFIN_MUMMY, drawn as a mummy on its spawn tile.
void Dungeon::drawDecoys(const CellRect& drawn) {
	const MonsterType& mummy = (*sim.monsterTypes)[MonsterMummy];
	const CharacterModel& model = sim.assets->monsterModels[MonsterMummy];
	for (int j = drawn.row0; j < drawn.row0 + drawn.rows; j++)
		for (int i = drawn.col0; i < drawn.col0 + drawn.cols; i++) {
			if (!IsInBounds(i, j) || decoration.decor[MapIndex(i, j)].type != DECOR_COFFIN_MUMMY)
				continue;
			glPushMatrix();
			glTranslatef(
				RenderConfig::TILE_SIZE * static_cast<float>(i - view().firstCol()),
				RenderConfig::TILE_SIZE * static_cast<float>(j - view().originRow),
				0
			);
			glTranslatef(RenderConfig::MONSTER_OFFSET_X, 0, RenderConfig::MONSTER_OFFSET_Z);
			DrawEntombed(mummy, model);
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
		if (a.fire && a.stuckMs < 0) // the tip burns
			Fire::draw(
				Fire::ARROW,
				RenderConfig::TILE_SIZE * (x - firstCol),
				RenderConfig::TILE_SIZE * (y - firstRow),
				ARROW_DEPTH,
				static_cast<uint32_t>(a.startMs)
			);
	}
}
//======================================================================================
// The dung balls (plan scarab-dung-ball): the rolling ones turning as they go, and the one each roller shapes in its
// windup, growing at the edge of its box. Broken, a ball is gone (its crumbs: drawBallEffects).
void Dungeon::drawBalls() {
	AnimatedModel* model = sim.assets->items.dungBall.get();
	if (!model)
		return;
	const auto firstCol = static_cast<float>(view().firstCol()); // DrawMonsters' frame
	const auto firstRow = static_cast<float>(view().originRow);
	const auto [low, high] = model->YRange(0);
	auto draw = [&](float x, float y, float radius, float turnDeg) {
		const float scale = 2.f * radius * RenderConfig::TILE_SIZE / std::max(high - low, 0.001f);
		glPushMatrix();
		glTranslatef(
			RenderConfig::TILE_SIZE * (x - firstCol),
			RenderConfig::TILE_SIZE * (y - firstRow + radius),
			RenderConfig::MONSTER_DEPTH
		);
		glRotatef(turnDeg, 0, 0, 1);
		glScalef(scale, scale, scale);
		glTranslatef(0, -(low + high) / 2.f, 0);
		model->Show();
		glPopMatrix();
	};
	for (const DungBall& b : balls)
		if (b.breakMs < 0) {
			const float r = b.rules.radius;
			draw(b.x, b.y, r, -static_cast<float>(b.dir) * b.rolled / r * RAD_TO_DEG);
		}
	for (const Monster& mon : monsters) {
		const float shaped = mon.Active() ? mon.BallShaped() : 0.f;
		if (shaped <= 0.f)
			continue;
		const float r = mon.BallRules().radius * (DUNG_BALL_SHAPED_FROM + (1.f - DUNG_BALL_SHAPED_FROM) * shaped);
		draw(mon.BallX(), static_cast<float>(mon.Row()), r, static_cast<float>(mon.BallDir()) * 200.f * shaped);
	}
}
//======================================================================================
// Khepri's burning sun on its way, and the crumbs where a ball broke. Same window and frame as drawSummonEffects.
void Dungeon::drawBallEffects() {
	const auto col0 = static_cast<float>(view().originCol), row0 = static_cast<float>(view().originRow);
	const int now = GameClock::now();
	for (const DungBall& b : balls) {
		const RollRules& rules = b.rules;
		const float x = RenderConfig::TILE_SIZE * (b.x - col0), y = RenderConfig::TILE_SIZE * (b.y - row0);
		const auto seed = static_cast<uint32_t>(b.startMs);
		if (b.breakMs < 0) {
			if (rules.burns()) // in front of the ball, or it hides them
				Fire::draw(
					Fire::SUN_BALL,
					x,
					y + RenderConfig::TILE_SIZE * rules.radius,
					RenderConfig::MONSTER_DEPTH + RenderConfig::TILE_SIZE * rules.radius,
					seed
				);
		} else if (!b.sank && now - b.breakMs < Grit::BURST_MS) {
			Grit::burst(x, y, RenderConfig::MONSTER_DEPTH, now - b.breakMs, false, seed);
			if (rules.burns())
				Fire::draw(
					Fire::BOLT_SPLAT,
					x,
					y,
					RenderConfig::MONSTER_DEPTH,
					seed,
					1.f - static_cast<float>(now - b.breakMs) / static_cast<float>(Grit::BURST_MS)
				);
		}
	}
	for (const Monster& mon : monsters) { // a burning one catches fire as it is shaped: the tell
		const float shaped = mon.Active() ? mon.BallShaped() : 0.f;
		if (shaped < 0.5f || !mon.BallRules().burns())
			continue;
		const float r = mon.BallRules().radius * (DUNG_BALL_SHAPED_FROM + (1.f - DUNG_BALL_SHAPED_FROM) * shaped);
		Fire::draw(
			Fire::SUN_BALL,
			RenderConfig::TILE_SIZE * (mon.BallX() - col0),
			RenderConfig::TILE_SIZE * (static_cast<float>(mon.Row()) + r - row0),
			RenderConfig::MONSTER_DEPTH + RenderConfig::TILE_SIZE * r,
			static_cast<uint32_t>(mon.Col()),
			2.f * shaped - 1.f
		);
	}
}
// The burning balls' light; same frame as addLights.
void Dungeon::addBallLights() {
	for (const DungBall& b : balls)
		if (b.breakMs < 0 && b.rules.burns())
			Lighting::add(
				RenderConfig::TILE_SIZE * (b.x - static_cast<float>(view().originCol)),
				RenderConfig::TILE_SIZE * (b.y + b.rules.radius - static_cast<float>(view().originRow)),
				RenderConfig::MONSTER_DEPTH + GLOB_LIGHT_LIFT,
				FIRE_BOLT_LIGHT,
				static_cast<uint32_t>(b.startMs)
			);
}
//======================================================================================
// The bow's arrow, smaller, pointing at the camera, from the back wall (-TILE_SIZE) out to the front (0).
void Dungeon::drawDarts() {
	AnimatedModel* model = sim.assets->items.missiles[static_cast<size_t>(MissileKind::Arrow)].get();
	if (dartTraps.darts.empty() || !model)
		return;
	const auto firstCol = static_cast<float>(view().firstCol()); // DrawMonsters' frame
	const auto firstRow = static_cast<float>(view().originRow);
	constexpr float DART_SCALE = 0.8f; // of the arrow model
	const float scale = ARROW_WORLD_PER_METRE * Ink::figureScale() * DART_SCALE;
	const float high = model->YRange(0).second;
	for (const DartTraps::Dart& d : dartTraps.darts) {
		glPushMatrix();
		glTranslatef(
			RenderConfig::TILE_SIZE * (d.x - firstCol),
			RenderConfig::TILE_SIZE * (d.y - firstRow),
			-RenderConfig::TILE_SIZE * (1.f - d.depth)
		);
		glRotatef(90.f, 1, 0, 0); // the model points up (+y): now at the camera (+z), the tip at the position
		glTranslatef(0, -high * scale, 0);
		glScalef(scale, scale, scale);
		model->Show();
		glPopMatrix();
	}
}
//======================================================================================
// A glob of venom (a bolt of light): with poison a cloud of green flames trailing smaller ones; with fire a fireball;
// else a small ball in its shooter's colour, stretched along its flight, flattening into a splat on a wall or the
// floor.
void Dungeon::drawShots() {
	const auto firstCol = static_cast<float>(view().firstCol()); // DrawMonsters' frame
	const auto firstRow = static_cast<float>(view().originRow);
	const int now = GameClock::now();
	for (const MonsterShot& v : shots) {
		float stretch = 1.6f, squash = 1.f, alpha = 0.85f;
		if (v.splatMs >= 0) {
			const float k = std::min(static_cast<float>(now - v.splatMs) / static_cast<float>(SHOT_SPLAT_MS), 1.f);
			stretch = 1.f + 1.5f * k;
			squash = 1.f - 0.7f * k;
			alpha *= 1.f - k;
		}
		drawGlobWithFlames(
			sim.assets->textures.nullTex,
			v.colour,
			globLook(v.from->ranged),
			static_cast<uint32_t>(v.spawnMs),
			RenderConfig::TILE_SIZE * (v.x - firstCol),
			RenderConfig::TILE_SIZE * (v.y - firstRow),
			ARROW_DEPTH,
			v.vx,
			v.vy,
			stretch,
			squash,
			alpha
		);
	}
}
// The light of each flying glob, venom or fire; same frame as addLights.
void Dungeon::addVenomLights() {
	for (const MonsterShot& v : shots) {
		const GlobLook& look = globLook(v.from->ranged);
		if (v.splatMs < 0 && look.light != nullptr)
			Lighting::add(
				RenderConfig::TILE_SIZE * (v.x - static_cast<float>(view().originCol)),
				RenderConfig::TILE_SIZE * (v.y - static_cast<float>(view().originRow)),
				ARROW_DEPTH + GLOB_LIGHT_LIFT,
				*look.light,
				static_cast<uint32_t>(v.spawnMs)
			);
	}
}
// The fire arrows' and the burning monsters' light; same frame as addLights.
void Dungeon::addFireLights() {
	const auto col0 = static_cast<float>(view().originCol), row0 = static_cast<float>(view().originRow);
	for (const Missile& a : missiles)
		if (a.fire && a.stuckMs < 0)
			Lighting::add(
				RenderConfig::TILE_SIZE * (a.X() - col0),
				RenderConfig::TILE_SIZE * (a.Y() - row0),
				ARROW_DEPTH + GLOB_LIGHT_LIFT,
				FIRE_ARROW_LIGHT,
				static_cast<uint32_t>(a.startMs)
			);
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Burns())
			Lighting::add(
				RenderConfig::TILE_SIZE * (mon.CentreX() - col0),
				RenderConfig::TILE_SIZE * ((mon.BottomY() + mon.TopY()) / 2.f - row0),
				RenderConfig::MONSTER_DEPTH + BURNING_FRONT,
				BURNING_LIGHT,
				static_cast<uint32_t>(mon.Col())
			);
}
//======================================================================================
// Flames over each burning monster's box. Same window and frame as drawSummonEffects.
void Dungeon::drawBurning() {
	const auto col0 = static_cast<float>(view().originCol), row0 = static_cast<float>(view().originRow);
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Burns())
			Fire::body(
				RenderConfig::TILE_SIZE * (mon.CentreX() - col0),
				RenderConfig::TILE_SIZE * (mon.BottomY() - row0),
				RenderConfig::MONSTER_DEPTH + BURNING_FRONT,
				RenderConfig::TILE_SIZE * mon.HalfWidth(),
				RenderConfig::TILE_SIZE * (mon.TopY() - mon.BottomY()),
				static_cast<uint32_t>(mon.Col() * 131 + mon.SpawnRow())
			);
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
		// Out of a wall: the dust at the wall's face, where the stone gives.
		const float wallFace = static_cast<float>(mon.Col()) + (mon.LocalX() > 0.f ? 1.f : 0.f); // a wall to the right
		const float from = mon.SummonedBy() == Summon::Wall ? wallFace : mon.CentreX();
		const float x = (from - static_cast<float>(view().originCol)) * RenderConfig::TILE_SIZE;
		const bool drop = mon.SummonedBy() == Summon::Drop;
		const float y = static_cast<float>(mon.Row() - view().originRow + (drop ? 1 : 0)) * RenderConfig::TILE_SIZE;
		Grit::burst(x, y, RenderConfig::MONSTER_DEPTH, age, drop, static_cast<uint32_t>(mon.SummonedMs() + mon.Col()));
	}
	holes.erase(
		std::remove_if(holes.begin(), holes.end(), [now](const Hole& h) { return now - h.startMs >= BURROW_HOLE_MS; }),
		holes.end()
	);
	for (const Hole& h : holes) {
		const float x = (h.x - static_cast<float>(view().originCol)) * RenderConfig::TILE_SIZE;
		const float y = static_cast<float>(h.row - view().originRow) * RenderConfig::TILE_SIZE;
		const int age = now - h.startMs;
		if (age < Grit::BURST_MS)
			Grit::burst(
				x,
				y,
				RenderConfig::MONSTER_DEPTH,
				age,
				false,
				static_cast<uint32_t>(h.startMs) + static_cast<uint32_t>(h.row)
			);
		drawHole(
			x,
			y + 0.3f,
			RenderConfig::MONSTER_DEPTH,
			1.f - static_cast<float>(age) / static_cast<float>(BURROW_HOLE_MS)
		);
	}
	// Rerek's spray as he comes up (plan worm-boss): sand thrown up all round him.
	sprays.erase(
		std::remove_if(
			sprays.begin(),
			sprays.end(),
			[now](const Hole& h) { return now - h.startMs >= Grit::BURST_MS; }
		),
		sprays.end()
	);
	for (const Hole& h : sprays)
		for (int k = -2; k <= 2; k++) {
			const float x = (h.x + SPRAY_SPREAD * static_cast<float>(k) - static_cast<float>(view().originCol)) *
							RenderConfig::TILE_SIZE;
			const float y = static_cast<float>(h.row - view().originRow) * RenderConfig::TILE_SIZE;
			Grit::burst(
				x,
				y,
				RenderConfig::MONSTER_DEPTH,
				now - h.startMs,
				false,
				static_cast<uint32_t>(h.startMs) + static_cast<uint32_t>(k + 2) * 7919U
			);
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
