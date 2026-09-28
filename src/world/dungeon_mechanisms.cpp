#include "dungeon.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../graphics/render_config.h"
#include "../graphics/fire.h"
#include "../input/gameplay_config.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
// Placement in prop space (tile units, origin = floor centre of the tile on the back wall, z towards the camera),
// from the extents mechanism.py prints.
constexpr float KEY_HOVER = 0.35f;					  // key height above the floor
constexpr float KEY_BOB = 0.04f;					  // up and down while it spins
constexpr float LEVER_PIVOT[3] = {0.f, 0.42f, 0.05f}; // handle pivot on the plate
constexpr float LEVER_ANGLE = 35.f;					  // degrees either side of upright; + tips the grip left
constexpr float ROCK_START_Y = 0.65f;				  // rock bottom when it breaks loose (rock is 0.3 tall)
constexpr float ROCK_DEPTH = 0.5f;					  // in the middle of the corridor, where the player walks

constexpr float GATE_APPROACH = 0.45f; // tiles from the gate at which a held key opens it

int lockBit(int colour) { return 1 << (colour - 1); }

float motionProgress(int startMs, int durationMs) {
	return std::clamp(static_cast<float>(GameClock::now() - startMs) / static_cast<float>(durationMs), 0.f, 1.f);
}

void showStatus(const char* text) {
	snprintf(GAME_STATE.status, sizeof(GAME_STATE.status), "%s", text);
	GAME_STATE.status_timer->Reset();
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

AnimatedModel* colourModel(std::unique_ptr<AnimatedModel> (&models)[LOCK_COLOUR_COUNT], int colour) {
	return isLockColour(colour) ? models[colour - 1].get() : nullptr;
}
} // namespace

void Dungeon::resetMechanisms() {
	keysHeld = 0;
	openingGates.clear();
	fallingRocks.clear();
	// A save taken mid-motion finishes it on load.
	for (Tile& t : map)
		if ((t.type == Gate || t.type == RockFall) && t.value == 2)
			t.value = 1;
}
//======================================================================================
void Dungeon::updateMechanisms() {
	int col = static_cast<int>(mapX);
	int row = static_cast<int>(mapY);
	Tile here = MapAt(col, row);

	if (here.type == Key && isLockColour(here.attr) && GAME_STATE.player->Alive()) {
		keysHeld |= lockBit(here.attr);
		map[MapIndex(col, row)] = Tile{Empty, 0, 0};
		char text[64];
		snprintf(text, sizeof(text), "Found the %s key", LOCK_GEM_NAMES[here.attr - 1]);
		showStatus(text);
		GAME_STATE.sounds.keyPickup.Play();
	}

	if (here.type == RockFall && here.value == 0 && GAME_STATE.player->Alive()) {
		map[MapIndex(col, row)].value = 2;
		fallingRocks.push_back({MapIndex(col, row), GameClock::now()});
		GAME_STATE.sounds.rockRumble.Play();
	}

	// A gate the player holds the key for opens as they come up to it.
	for (int dir : {-1, 1}) {
		Tile next = MapAt(col + dir, row);
		float gap = dir > 0 ? static_cast<float>(col + 1) - mapX : mapX - static_cast<float>(col);
		if (next.type == Gate && next.value == 0 && isLockColour(next.attr) && (keysHeld & lockBit(next.attr)) != 0 &&
			gap < GATE_APPROACH) {
			startOpeningGate(MapIndex(col + dir, row));
			GAME_STATE.sounds.gateOpen.Play();
		}
	}

	for (auto it = openingGates.begin(); it != openingGates.end();) {
		if (motionProgress(it->startMs, GATE_OPEN_MS) >= 1.f) {
			map[it->cell].value = 1;
			it = openingGates.erase(it);
		} else
			++it;
	}

	updateRocks();
}
//======================================================================================
void Dungeon::updateRocks() {
	for (auto it = fallingRocks.begin(); it != fallingRocks.end();) {
		if (GameClock::now() - it->startMs < ROCK_WARN_MS + ROCK_FALL_MS) {
			++it;
			continue;
		}

		// Landed: hits the player if they are still under it.
		int col = it->cell % MAP_WIDTH;
		int row = it->cell / MAP_WIDTH;
		float centreX = static_cast<float>(col) + 0.5f;
		auto floorY = static_cast<float>(row);
		GAME_STATE.sounds.rockCrash.Play();
		float dx = std::fabs(mapX - centreX);
		if (dx < ROCK_GRAZE_HALF_WIDTH && mapY >= floorY - 0.2f && mapY < floorY + ROCK_HIT_HEIGHT &&
			GAME_STATE.player->Alive()) {
			bool crushed = dx < ROCK_CRUSH_HALF_WIDTH;
			GAME_STATE.player->TakeHit(crushed ? ROCK_CRUSH_DAMAGE : ROCK_GRAZE_DAMAGE, true);
			showStatus(crushed ? "Crushed by a falling rock!" : "The rock clips your leg!");
		}
		map[it->cell].value = 1;
		it = fallingRocks.erase(it);
	}
}
//======================================================================================
void Dungeon::startOpeningGate(int cell) {
	if (map[cell].type != Gate || map[cell].value != 0)
		return;
	map[cell].value = 2;
	openingGates.push_back({cell, GameClock::now()});
}
//======================================================================================
void Dungeon::openGates(int colour) {
	bool any = false;
	for (int cell = 0; cell < MAP_WIDTH * MAP_HEIGHT; cell++)
		if (map[cell].type == Gate && map[cell].attr == colour && map[cell].value == 0) {
			startOpeningGate(cell);
			any = true;
		}
	if (any)
		GAME_STATE.sounds.gateOpen.Play();
}
//======================================================================================
void Dungeon::bumpGate(int col, int row) {
	Tile gate = MapAt(col, row);
	if (gate.type != Gate || gate.value != 0 || !isLockColour(gate.attr))
		return;

	if ((keysHeld & lockBit(gate.attr)) != 0) {
		startOpeningGate(MapIndex(col, row));
		GAME_STATE.sounds.gateOpen.Play();
		return;
	}

	int now = GameClock::now();
	if (now - lockedHintMs < LOCKED_HINT_INTERVAL_MS)
		return;
	lockedHintMs = now;
	char text[96];
	snprintf(text, sizeof(text), "Sealed. It needs the %s key or a %s lever.", LOCK_GEM_NAMES[gate.attr - 1],
			 LOCK_GEM_NAMES[gate.attr - 1]);
	showStatus(text);
	GAME_STATE.sounds.gateLocked.Play();
}
//======================================================================================
bool Dungeon::PullLever() {
	int col = static_cast<int>(mapX);
	int row = static_cast<int>(mapY);
	Tile lever = MapAt(col, row);
	if (lever.type != Lever || !isLockColour(lever.attr))
		return false;
	if (lever.value != 0)
		return true; // pulled already: the gates stay open

	map[MapIndex(col, row)].value = 1;
	GAME_STATE.sounds.lever.Play();
	openGates(lever.attr);
	char text[64];
	snprintf(text, sizeof(text), "Somewhere a %s gate grinds open", LOCK_GEM_NAMES[lever.attr - 1]);
	showStatus(text);
	return true;
}
//======================================================================================
void Dungeon::drawKeyTile(int i, int j) {
	Tile tile = MapAt(i, j);
	float t = static_cast<float>(GameClock::now());
	glPushMatrix();
	enterPropSpace();
	glTranslatef(0, KEY_HOVER + KEY_BOB * std::sin(t * 0.004f + static_cast<float>(i)), 0.5f);
	glRotatef(std::fmod(t * KEY_SPIN_DEG_PER_MS, 360.f), 0, 1, 0);
	showModel(colourModel(GAME_STATE.mechanisms.key, tile.attr));
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawGateTile(int i, int j) {
	Tile tile = MapAt(i, j);
	float lift = 0.f;
	if (tile.value == 1)
		lift = 1.f;
	else if (tile.value == 2)
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
	showModel(colourModel(GAME_STATE.mechanisms.gate, tile.attr));
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawLeverTile(int i, int j) {
	Tile tile = MapAt(i, j);
	glPushMatrix();
	enterPropSpace();
	showModel(colourModel(GAME_STATE.mechanisms.leverBase, tile.attr));
	glTranslatef(LEVER_PIVOT[0], LEVER_PIVOT[1], LEVER_PIVOT[2]);
	glRotatef(tile.value != 0 ? -LEVER_ANGLE : LEVER_ANGLE, 0, 0, 1); // pulled = handle turned to the right
	showModel(GAME_STATE.mechanisms.leverHandle.get());
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawRockFallTile(int i, int j) {
	Tile tile = MapAt(i, j);
	int cell = MapIndex(i, j);

	glPushMatrix();
	enterPropSpace();
	if (tile.value != 1) {
		// Loose stones in the ceiling; they shake while the rumble lasts.
		glPushMatrix();
		if (tile.value == 2) {
			float t = static_cast<float>(GameClock::now());
			glTranslatef(0.01f * std::sin(t * 0.09f), 0.f, 0.f);
		}
		showModel(GAME_STATE.mechanisms.crack.get());
		glPopMatrix();
	}

	float rockY = -1.f; // below the floor = not drawn
	if (tile.value == 1)
		rockY = 0.f;
	else if (tile.value == 2)
		for (const Motion& m : fallingRocks)
			if (m.cell == cell && GameClock::now() - m.startMs >= ROCK_WARN_MS) {
				float p = motionProgress(m.startMs + ROCK_WARN_MS, ROCK_FALL_MS);
				rockY = ROCK_START_Y * (1.f - p * p); // accelerates down
			}
	if (rockY >= 0.f) {
		glTranslatef(0, rockY, ROCK_DEPTH);
		glRotatef(static_cast<float>(cell % 7) * 50.f, 0, 1, 0); // fallen rocks do not all look the same
		showModel(GAME_STATE.mechanisms.rock.get());
	}
	glPopMatrix();
}
//======================================================================================
// Same window and frame as the tile loop in Draw(): cell (col0, row0) sits at the origin.
void Dungeon::drawMechanismEffects() {
	int col0 = static_cast<int>(mapX) - 3;
	int row0 = static_cast<int>(mapY) - 3;
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
