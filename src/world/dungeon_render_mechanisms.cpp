// Drawing the keys, the expedition's pages, gates, levers, rock falls, dart traps, wall spikes and fire vents
// (dungeon_mechanisms.cpp, dungeon_darts.cpp and dungeon_spikes.cpp have their rules).
#include "dungeon.h"
#include "dungeon_rules.h"
#include "../state/assets.h"
#include "../graphics/render_config.h"
#include "../graphics/fire.h"
#include "../graphics/lighting.h"
#include "spike_cycle.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <memory>

namespace {
// Placement in prop space (tile units, origin = floor centre of the tile on the back wall, z towards the camera),
// from the extents mechanism.py prints.
constexpr float KEY_HOVER = 0.35f; // key height above the floor
constexpr float KEY_BOB = 0.04f; // up and down while it spins
constexpr float PAGE_HOVER = 0.42f; // a torn page over the skeleton it lay on (plan expedition-notes)
constexpr float PAGE_W = 0.2f; // the texture's shape, 256 x 320
constexpr float PAGE_H = 0.25f;
constexpr float PAGE_SPIN_DEG_PER_MS = 0.05f; // slower than a key: paper drifting round
constexpr float PAGE_SWAY_DEG = 12.f; // and rocking a little
constexpr float LEVER_PIVOT[3] = {0.f, 0.42f, 0.05f}; // handle pivot on the plate
constexpr float LEVER_ANGLE = 35.f; // degrees either side of upright; + tips the grip left
constexpr float ROCK_START_Y = 0.65f; // rock bottom when it breaks loose (rock is 0.3 tall)
constexpr float ROCK_DEPTH = 0.5f; // in the middle of the corridor, where the player walks
constexpr float DART_PLATE_SINK = 0.025f; // a pressed plate is flush with the floor
// Wall spikes: pushed this far back into the wall, their tips are just inside it. Out, they shoot out in
// SPIKES_THRUST_MS and slide back in over the last SPIKES_RETRACT_MS of their time out; in the telegraph the tips
// twitch out of the slots, up to SPIKES_TWITCH.
constexpr float SPIKES_HIDE_DEPTH = 1.f;
constexpr int SPIKES_THRUST_MS = 80;
constexpr int SPIKES_RETRACT_MS = 250;
constexpr float SPIKES_TWITCH = 0.09f;

// A fire vent's jet (plan fire-jet-trap): out of the mouth low in the back wall (mechanism.py's VENT_TOP), it rolls
// forward to the corridor's middle and up to the ceiling, JET_PLUMES fires stacked up the cell. It flares up in
// JET_FLARE_MS and dies down over the last JET_FADE_MS of its time out. In the telegraph, embers glow in the mouth,
// brighter as it nears.
constexpr float VENT_MOUTH_Y = 0.13f * RenderConfig::TILE_SIZE;
constexpr float VENT_MOUTH_Z = -0.94f * RenderConfig::TILE_SIZE;
constexpr int JET_PLUMES = 4;
constexpr int JET_FLARE_MS = 80;
constexpr int JET_FADE_MS = 250;
constexpr FireStyle JET = {64, 520.f, 7.f, 26.f, 8.f, 2.f, 0.2f};
constexpr FireStyle VENT_EMBERS = {14, 320.f, 5.f, 5.f, 2.6f, 0.6f, 0.f};
constexpr Lighting::LightDef JET_LIGHT = {1.f, 0.5f, 0.18f, 150.f, 0.8f};
constexpr Lighting::LightDef VENT_GLOW = {0.9f, 0.28f, 0.08f, 45.f, 0.6f};

// How strong the jet is drawn, 0 (none) to 1, at that point of its cycle; the telegraph's glow, 0 to 1.
float jetStrength(const Fixture& t, int nowMs) {
	const int at = spikeCycleMs(t, nowMs);
	const int out = spikeOutMs(t);
	if (at >= out)
		return 0.f;
	return std::min({1.f, static_cast<float>(at + 1) / JET_FLARE_MS, static_cast<float>(out - at) / JET_FADE_MS});
}
float ventGlow(const Fixture& t, int nowMs) {
	const int toOut = spikePeriodMs(t) - spikeCycleMs(t, nowMs);
	return toOut > SPIKES_TELEGRAPH_MS || spikeCycleMs(t, nowMs) < spikeOutMs(t)
			   ? 0.f
			   : 1.f - static_cast<float>(toOut) / SPIKES_TELEGRAPH_MS;
}

// How far out the spikes are drawn, 0 (in the wall) to 1, at that point of their cycle.
float spikesOut(const Fixture& t, int nowMs) {
	const int at = spikeCycleMs(t, nowMs);
	const int out = spikeOutMs(t);
	if (at < SPIKES_THRUST_MS)
		return static_cast<float>(at) / SPIKES_THRUST_MS;
	if (at < out)
		return std::min(1.f, static_cast<float>(out - at) / SPIKES_RETRACT_MS);
	const int toOut = spikePeriodMs(t) - at;
	if (toOut > SPIKES_TELEGRAPH_MS)
		return 0.f;
	const float rise = 1.f - static_cast<float>(toOut) / SPIKES_TELEGRAPH_MS; // the twitches come faster, further
	return SPIKES_TWITCH * rise * std::fabs(std::sin(static_cast<float>(at) * 0.03f * (1.f + rise)));
}

// Prop frame of the current tile (see Dungeon::drawDecorTile).
void enterPropSpace() {
	glTranslatef(RenderConfig::TILE_HALF, 0, -RenderConfig::TILE_SIZE);
	glScalef(RenderConfig::TILE_SIZE, RenderConfig::TILE_SIZE, RenderConfig::TILE_SIZE);
}

void showModel(AnimatedModel* model) {
	if (model != nullptr)
		model->Show(); // textured only, lighting is baked in (like the props)
}

// A model shared by the lock colours, with the colour's texture. Nothing for a tile without a lock colour.
void showColour(const std::unique_ptr<AnimatedModel>& model, const Texture (&textures)[LOCK_COLOUR_COUNT], int colour) {
	if (model != nullptr && isLockColour(colour))
		model->Show(AnimPlayback{}, textures[colour - 1].ID());
}
} // namespace

void Dungeon::drawKeyTile(int i, int j) {
	Tile tile = MapAt(i, j);
	float t = static_cast<float>(GameClock::now());
	glPushMatrix();
	enterPropSpace();
	glTranslatef(0, KEY_HOVER + KEY_BOB * std::sin(t * 0.004f + static_cast<float>(i)), 0.5f);
	glRotatef(std::fmod(t * KEY_SPIN_DEG_PER_MS, 360.f), 0, 1, 0);
	showColour(sim.assets->mechanisms.key, sim.assets->mechanisms.keyTex, tile.attr);
	glPopMatrix();
}
//======================================================================================
// Like a key: floating over its cell, turning, so it reads as something to pick up. Nothing once taken (the skeleton
// under it is decor).
void Dungeon::drawPageTile(int i, int j) {
	if (MapAt(i, j).value != 0)
		return;
	const auto t = static_cast<float>(GameClock::now());
	const auto phase = static_cast<float>(i * 7 + j);
	glPushMatrix();
	enterPropSpace();
	glTranslatef(0, PAGE_HOVER + KEY_BOB * std::sin(t * 0.003f + phase), 0.5f);
	glRotatef(std::fmod(t * PAGE_SPIN_DEG_PER_MS, 360.f), 0, 1, 0);
	glRotatef(PAGE_SWAY_DEG * std::sin(t * 0.0021f + phase), 0, 0, 1);
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
	glDisable(GL_CULL_FACE);
	glEnable(GL_ALPHA_TEST); // the torn edge
	glAlphaFunc(GL_GREATER, 0.5f);
	glEnable(GL_TEXTURE_2D);
	sim.assets->textures.expeditionPage.Bind();
	glColor3f(1, 1, 1);
	glNormal3f(0, 0, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);
	glVertex3f(-PAGE_W / 2, -PAGE_H / 2, 0);
	glTexCoord2f(1, 1);
	glVertex3f(PAGE_W / 2, -PAGE_H / 2, 0);
	glTexCoord2f(1, 0);
	glVertex3f(PAGE_W / 2, PAGE_H / 2, 0);
	glTexCoord2f(0, 0);
	glVertex3f(-PAGE_W / 2, PAGE_H / 2, 0);
	glEnd();
	glPopAttrib();
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawGateTile(int i, int j) {
	Tile tile = MapAt(i, j);
	float lift = 0.f;
	if (gateState(tile) == GateState::Open)
		lift = 1.f;
	else if (gateState(tile) == GateState::Opening)
		for (const Motion& m : openingGates)
			if (m.cell == MapIndex(i, j)) {
				float p = motionProgress(m.startMs, GATE_OPEN_MS);
				lift = p * p * (3.f - 2.f * p);
			}
	if (lift >= 1.f)
		return; // up inside the ceiling

	glPushMatrix();
	enterPropSpace();
	glTranslatef(0, lift, 0);
	const MechanismSet& set = sim.assets->mechanisms;
	if (tile.attr == BOSS_LOCK) {
		if (set.gate != nullptr)
			set.gate->Show(AnimPlayback{}, set.bossGateTex.ID());
	} else
		showColour(set.gate, set.gateTex, tile.attr);
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawLeverTile(int i, int j) {
	const Fixture lever = MapAt(i, j).fixture;
	glPushMatrix();
	enterPropSpace();
	showColour(sim.assets->mechanisms.leverBase, sim.assets->mechanisms.leverBaseTex, lever.attr);
	glTranslatef(LEVER_PIVOT[0], LEVER_PIVOT[1], LEVER_PIVOT[2]);
	glRotatef(leverPulled(lever) ? -LEVER_ANGLE : LEVER_ANGLE, 0, 0, 1); // pulled = handle turned to the right
	showModel(sim.assets->mechanisms.leverHandle.get());
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawRockFallTile(int i, int j) {
	Tile tile = MapAt(i, j);
	int cell = MapIndex(i, j);

	glPushMatrix();
	enterPropSpace();
	if (rockState(tile) != RockState::Fallen) {
		// Loose stones in the ceiling; they shake while the rumble lasts.
		glPushMatrix();
		if (rockState(tile) == RockState::Falling) {
			float t = static_cast<float>(GameClock::now());
			glTranslatef(0.01f * std::sin(t * 0.09f), 0.f, 0.f);
		}
		showModel(sim.assets->mechanisms.crack.get());
		glPopMatrix();
	}

	float rockY = -1.f; // below the floor = not drawn
	if (isSandFall(tile)) // no stone: the sand pours (drawSandEffects) and the cell turns to a drift
		rockY = -1.f;
	else if (rockState(tile) == RockState::Fallen)
		rockY = 0.f;
	else if (rockState(tile) == RockState::Falling)
		for (const Motion& m : fallingRocks)
			if (m.cell == cell && GameClock::now() - m.startMs >= ROCK_WARN_MS) {
				float p = motionProgress(m.startMs + ROCK_WARN_MS, ROCK_FALL_MS);
				rockY = ROCK_START_Y * (1.f - p * p); // accelerates down
			}
	if (rockY >= 0.f) {
		glTranslatef(0, rockY, ROCK_DEPTH);
		glRotatef(static_cast<float>(cell % 7) * 50.f, 0, 1, 0); // fallen rocks do not all look the same
		showModel(sim.assets->mechanisms.rock.get());
	}
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawDartPlateTile(int i, int j) {
	glPushMatrix();
	enterPropSpace();
	showModel(sim.assets->mechanisms.dartHoles.get());
	if (platePressed(MapAt(i, j)))
		glTranslatef(0, -DART_PLATE_SINK, 0);
	showModel(sim.assets->mechanisms.plate.get());
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawWallSpikesTile(int i, int j) {
	glPushMatrix();
	enterPropSpace();
	showModel(sim.assets->mechanisms.spikeSlots.get());
	const float out = spikesOut(MapAt(i, j).fixture, GameClock::now());
	if (out > 0.f) {
		glTranslatef(0, 0, -SPIKES_HIDE_DEPTH * (1.f - out));
		showModel(sim.assets->mechanisms.wallSpikes.get());
	}
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawFireVentTile() {
	glPushMatrix();
	enterPropSpace();
	showModel(sim.assets->mechanisms.fireVent.get());
	glPopMatrix();
}
//======================================================================================
// Same frame as the tile loop in Draw(): cell (col0, row0) of the window sits at the origin.
void Dungeon::drawFireJets(const CellRect& drawn) {
	const int now = GameClock::now();
	for (int j = drawn.row0; j < drawn.row0 + drawn.rows; j++)
		for (int i = drawn.col0; i < drawn.col0 + drawn.cols; i++) {
			if (!IsInBounds(i, j) || MapAt(i, j).fixture.type != FireVent)
				continue;
			const Fixture& f = MapAt(i, j).fixture;
			const float x = static_cast<float>(i - view().originCol) * RenderConfig::TILE_SIZE +
							RenderConfig::TILE_HALF;
			const float y = static_cast<float>(j - view().originRow) * RenderConfig::TILE_SIZE;
			const auto seed = static_cast<uint32_t>(MapIndex(i, j)) * 2654435761u;
			if (const float glow = ventGlow(f, now); glow > 0.f)
				Fire::draw(VENT_EMBERS, x, y + VENT_MOUTH_Y * 0.5f, VENT_MOUTH_Z, seed, glow);
			const float jet = jetStrength(f, now);
			for (int k = 0; k < JET_PLUMES && jet > 0.f; k++) { // rolling out of the mouth, then rising up the cell
				const float up = static_cast<float>(k) / (JET_PLUMES - 1);
				Fire::draw(
					JET,
					x,
					y + VENT_MOUTH_Y * 0.5f + up * 0.6f * RenderConfig::TILE_SIZE,
					VENT_MOUTH_Z + (RenderConfig::MONSTER_DEPTH - VENT_MOUTH_Z) * std::min(1.f, 2.f * up),
					seed + static_cast<uint32_t>(k) * 7919u,
					jet
				);
			}
		}
}
//======================================================================================
// Same frame as addLights.
void Dungeon::addFireVentLights(const CellRect& drawn) {
	const int now = GameClock::now();
	const CellRect lit = drawn.grown(2);
	for (int j = lit.row0; j < lit.row0 + lit.rows; j++)
		for (int i = lit.col0; i < lit.col0 + lit.cols; i++) {
			if (!IsInBounds(i, j) || MapAt(i, j).fixture.type != FireVent)
				continue;
			const Fixture& f = MapAt(i, j).fixture;
			const float x = static_cast<float>(i - view().originCol) * RenderConfig::TILE_SIZE +
							RenderConfig::TILE_HALF;
			const float y = static_cast<float>(j - view().originRow) * RenderConfig::TILE_SIZE;
			const auto seed = static_cast<uint32_t>(MapIndex(i, j));
			if (jetStrength(f, now) > 0.f)
				Lighting::add(x, y + RenderConfig::TILE_HALF, RenderConfig::MONSTER_DEPTH, JET_LIGHT, seed);
			else if (ventGlow(f, now) > 0.f)
				Lighting::add(x, y + VENT_MOUTH_Y, VENT_MOUTH_Z + 6.f, VENT_GLOW, seed);
		}
}
//======================================================================================
// Same window and frame as the tile loop in Draw(): cell (col0, row0) sits at the origin.
void Dungeon::drawMechanismEffects() {
	const int col0 = view().originCol;
	const int row0 = view().originRow;
	for (const Motion& m : fallingRocks) {
		int age = GameClock::now() - m.startMs;
		int col = m.cell % MAP_WIDTH;
		int row = m.cell / MAP_WIDTH;
		float x = static_cast<float>(col - col0) * RenderConfig::TILE_SIZE + RenderConfig::TILE_HALF;
		float y = static_cast<float>(row - row0) * RenderConfig::TILE_SIZE;
		float z = -RenderConfig::TILE_SIZE * (1.f - ROCK_DEPTH);
		Dust::draw(
			x,
			y + RenderConfig::TILE_SIZE,
			z,
			static_cast<float>(age) / (ROCK_WARN_MS + ROCK_FALL_MS),
			static_cast<uint32_t>(m.cell)
		);
	}
}
