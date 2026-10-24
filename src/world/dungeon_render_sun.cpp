// Drawing the sunlight mirrors (plan sunlight-mirrors; dungeon_sun.cpp has their rules): the mirrors on their stands,
// the sun discs, the beams and their light.
#include "dungeon.h"
#include "dungeon_rules.h"
#include "../state/assets.h"
#include "../graphics/fire.h"
#include "../graphics/lighting.h"
#include "../graphics/render_config.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr float T = RenderConfig::TILE_SIZE;
// In tile units (prop space: origin the floor centre of the cell on the back wall, z towards the camera), from the
// extents mechanism.py prints: the beam runs this far in front of the back wall, this high over the floor, through the
// mirror's pivot.
constexpr float BEAM_DEPTH = 0.3f;
constexpr float BEAM_HEIGHT = 0.5f;
constexpr float POINTER_AHEAD = 0.07f; // the pointer turns on the hub, in front of the mirror
constexpr float BEAM_Z = -T * (1.f - BEAM_DEPTH); // in the level frame

float dirDeg(BeamDir d) {
	return std::atan2(-static_cast<float>(beamDx(d)), static_cast<float>(beamDy(d))) * 57.29578f;
}

// Map point of a cell's edge the way d goes from its centre line: a column runs through the middle, a row at
// BEAM_HEIGHT.
void edgePoint(int cell, BeamDir d, float& x, float& y) {
	const int col = cell % LEVEL_WIDTH, row = cell / LEVEL_WIDTH;
	x = static_cast<float>(col) + 0.5f + 0.5f * static_cast<float>(beamDx(d));
	y = static_cast<float>(row) + (beamDy(d) > 0 ? 1.f : (beamDy(d) < 0 ? 0.f : BEAM_HEIGHT));
}

void centrePoint(int cell, float& x, float& y) {
	const int col = cell % LEVEL_WIDTH, row = cell / LEVEL_WIDTH;
	x = static_cast<float>(col) + 0.5f;
	y = static_cast<float>(row) + BEAM_HEIGHT;
}

void enterPropSpace() {
	glTranslatef(RenderConfig::TILE_HALF, 0, -RenderConfig::TILE_SIZE);
	glScalef(RenderConfig::TILE_SIZE, RenderConfig::TILE_SIZE, RenderConfig::TILE_SIZE);
}
} // namespace

void Dungeon::drawMirrorTile(int i, int j) {
	const MechanismSet& set = sim.assets->mechanisms;
	const int cell = MapIndex(i, j);
	glPushMatrix();
	enterPropSpace();
	if (set.mirrorStand != nullptr)
		set.mirrorStand->Show();
	glTranslatef(0, BEAM_HEIGHT, BEAM_DEPTH);
	glPushMatrix();
	glRotatef(mirrorAngle(cell), 0, 0, 1);
	if (set.mirror != nullptr)
		set.mirror->Show();
	glPopMatrix();
	glTranslatef(0, 0, POINTER_AHEAD);
	glRotatef(dirDeg(mirrorDir(map[cell])), 0, 0, 1);
	if (set.mirrorPointer != nullptr)
		set.mirrorPointer->Show();
	glPopMatrix();
}
//======================================================================================
void Dungeon::drawSunDiscTile(int i, int j) {
	const MechanismSet& set = sim.assets->mechanisms;
	const Fixture disc = MapAt(i, j).fixture;
	if (set.sunDisc == nullptr || !isLockColour(disc.attr))
		return;
	glPushMatrix();
	enterPropSpace();
	set.sunDisc->Show(AnimPlayback{}, set.sunDiscTex[disc.attr - 1].ID());
	glPopMatrix();
}
//======================================================================================
// Same window and frame as drawSummonEffects.
void Dungeon::drawSunBeams() {
	const auto col0 = static_cast<float>(view().originCol), row0 = static_cast<float>(view().originRow);
	auto world = [&](float mx, float my, float& x, float& y) {
		x = (mx - col0) * T;
		y = (my - row0) * T;
	};
	float px = 0.f, py = 0.f; // the beam's last corner, map units
	auto lineTo = [&](float mx, float my, uint32_t seed) {
		float x0 = 0.f, y0 = 0.f, x1 = 0.f, y1 = 0.f;
		world(px, py, x0, y0);
		world(mx, my, x1, y1);
		SunBeam::ray(x0, y0, x1, y1, BEAM_Z, seed);
		px = mx;
		py = my;
	};
	auto glowAt = [&](float mx, float my, float size, float strength) {
		float x = 0.f, y = 0.f;
		world(mx, my, x, y);
		SunBeam::glow(x, y, BEAM_Z, size, strength);
	};
	bool fresh = true; // the next step starts a beam, at its light shaft
	for (const BeamStep& s : sunBeams) {
		if (fresh) {
			edgePoint(s.cell, opposite(s.in), px, py);
			glowAt(px, py, 22.f, 0.8f); // the crack the sun comes through
			fresh = false;
		}
		const bool mirror = map[s.cell].type == Mirror;
		float x = 0.f, y = 0.f;
		if (mirror && s.out != s.in) { // turned, or caught
			centrePoint(s.cell, x, y);
			lineTo(x, y, static_cast<uint32_t>(s.cell));
			glowAt(x, y, 14.f, 0.7f);
		}
		if (!s.ends)
			continue;
		fresh = true;
		if (mirror && s.out == opposite(s.in))
			continue;
		const int next = s.cell + beamDx(s.out) + beamDy(s.out) * MAP_WIDTH;
		const Tile ahead = IsInBounds(s.cell % MAP_WIDTH + beamDx(s.out), s.cell / MAP_WIDTH + beamDy(s.out))
							   ? map[next]
							   : wallTile();
		if (ahead.type == Gate && beamDy(s.out) == 0) // the portcullis stands in the middle of its cell
			centrePoint(next, x, y);
		else
			edgePoint(s.cell, s.out, x, y);
		lineTo(x, y, static_cast<uint32_t>(s.cell) * 7U);
		glowAt(x, y, 18.f, 0.6f); // where it falls on the rock
	}

	const int now = GameClock::now();
	for (int j = drawnCells.row0; j < drawnCells.row0 + drawnCells.rows; j++)
		for (int i = drawnCells.col0; i < drawnCells.col0 + drawnCells.cols; i++) {
			if (!IsInBounds(i, j) || MapAt(i, j).fixture.type != SunDisc || !sunDiscLit(MapAt(i, j).fixture))
				continue;
			float flare = 0.f;
			for (const Motion& m : litDiscs)
				if (m.cell == MapIndex(i, j))
					flare = 1.f - motionProgress(m.startMs, SUN_DISC_FLARE_MS);
			float x = 0.f, y = 0.f;
			world(static_cast<float>(i) + 0.5f, static_cast<float>(j) + BEAM_HEIGHT, x, y);
			const float pulse = 0.08f * std::sin(static_cast<float>(now) * 0.003f + static_cast<float>(i));
			SunBeam::glow(x, y, -T + 1.5f, 26.f + 40.f * flare, 0.45f + pulse + 1.2f * flare);
		}
}
//======================================================================================
// Same frame as addLights: along the beams every other cell and each corner, the lit sun discs.
void Dungeon::addSunLights(const CellRect& drawn) {
	const CellRect near = drawn.grown(2);
	auto inside = [&](int col, int row) {
		return col >= near.col0 && col < near.col0 + near.cols && row >= near.row0 && row < near.row0 + near.rows;
	};
	const auto col0 = static_cast<float>(view().originCol), row0 = static_cast<float>(view().originRow);
	for (size_t k = 0; k < sunBeams.size(); k++) {
		const BeamStep& s = sunBeams[k];
		const int col = s.cell % MAP_WIDTH, row = s.cell / MAP_WIDTH;
		if (!inside(col, row) || (k % 2 != 0 && s.in == s.out && !s.ends))
			continue;
		Lighting::add(
			(static_cast<float>(col) + 0.5f - col0) * T,
			(static_cast<float>(row) + BEAM_HEIGHT - row0) * T,
			BEAM_Z,
			Lighting::SUNBEAM,
			static_cast<uint32_t>(s.cell)
		);
	}
	for (int j = near.row0; j < near.row0 + near.rows; j++)
		for (int i = near.col0; i < near.col0 + near.cols; i++)
			if (IsInBounds(i, j) && MapAt(i, j).fixture.type == SunDisc && sunDiscLit(MapAt(i, j).fixture))
				Lighting::add(
					(static_cast<float>(i) + 0.5f - col0) * T,
					(static_cast<float>(j) + BEAM_HEIGHT - row0) * T,
					-T + 4.f,
					Lighting::SUNBEAM,
					static_cast<uint32_t>(MapIndex(i, j))
				);
}
