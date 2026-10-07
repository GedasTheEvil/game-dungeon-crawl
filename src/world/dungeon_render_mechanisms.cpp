// Drawing the keys, gates, levers and rock falls (dungeon_mechanisms.cpp has their rules).
#include "dungeon.h"
#include "dungeon_rules.h"
#include "../state/assets.h"
#include "../graphics/render_config.h"
#include "../graphics/fire.h"
#include <GL/gl.h>
#include <cmath>
#include <memory>

namespace {
// Placement in prop space (tile units, origin = floor centre of the tile on the back wall, z towards the camera),
// from the extents mechanism.py prints.
constexpr float KEY_HOVER = 0.35f;					  // key height above the floor
constexpr float KEY_BOB = 0.04f;					  // up and down while it spins
constexpr float LEVER_PIVOT[3] = {0.f, 0.42f, 0.05f}; // handle pivot on the plate
constexpr float LEVER_ANGLE = 35.f;					  // degrees either side of upright; + tips the grip left
constexpr float ROCK_START_Y = 0.65f;				  // rock bottom when it breaks loose (rock is 0.3 tall)
constexpr float ROCK_DEPTH = 0.5f;					  // in the middle of the corridor, where the player walks

// Prop frame of the current tile (see Dungeon::drawDecorTile).
void enterPropSpace() {
	glTranslatef(RenderConfig::TILE_HALF, 0, -RenderConfig::TILE_SIZE);
	glScalef(RenderConfig::TILE_SIZE, RenderConfig::TILE_SIZE, RenderConfig::TILE_SIZE);
}

void showModel(AnimatedModel* model) {
	if (model != nullptr)
		model->Show(); // textured only, lighting is baked in (like the props)
}

AnimatedModel* colourModel(std::unique_ptr<AnimatedModel> (&models)[LOCK_COLOUR_COUNT], int colour) {
	return isLockColour(colour) ? models[colour - 1].get() : nullptr;
}
} // namespace

void Dungeon::drawKeyTile(int i, int j) {
	Tile tile = MapAt(i, j);
	float t = static_cast<float>(GameClock::now());
	glPushMatrix();
	enterPropSpace();
	glTranslatef(0, KEY_HOVER + KEY_BOB * std::sin(t * 0.004f + static_cast<float>(i)), 0.5f);
	glRotatef(std::fmod(t * KEY_SPIN_DEG_PER_MS, 360.f), 0, 1, 0);
	showModel(colourModel(sim.assets->mechanisms.key, tile.attr));
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
	showModel(tile.attr == BOSS_LOCK ? sim.assets->mechanisms.bossGate.get()
									 : colourModel(sim.assets->mechanisms.gate, tile.attr));
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawLeverTile(int i, int j) {
	Tile tile = MapAt(i, j);
	glPushMatrix();
	enterPropSpace();
	showModel(colourModel(sim.assets->mechanisms.leverBase, tile.attr));
	glTranslatef(LEVER_PIVOT[0], LEVER_PIVOT[1], LEVER_PIVOT[2]);
	glRotatef(leverPulled(tile) ? -LEVER_ANGLE : LEVER_ANGLE, 0, 0, 1); // pulled = handle turned to the right
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
	if (rockState(tile) == RockState::Fallen)
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
		Dust::draw(x, y + RenderConfig::TILE_SIZE, z, static_cast<float>(age) / (ROCK_WARN_MS + ROCK_FALL_MS),
				   static_cast<uint32_t>(m.cell));
	}
}
