#ifndef SUN_BEAM_H
#define SUN_BEAM_H
#include "level.h"
#include <cstdint>
#include <functional>
#include <vector>

// Sunlight mirrors (plan sunlight-mirrors): a beam of sunlight runs from each light shaft through open cells in the
// level's plane, a bronze mirror sends it on the way it points, a sun disc it falls on is lit for good and opens the
// gates of its colour. The game (Dungeon::updateSunBeams) and the checker trace it the same way.

// A beam's way, as a light shaft's attribute and a mirror's value: a quarter turn clockwise (as the camera sees the
// level) from each to the next.
enum class BeamDir : std::uint8_t { Down = 0, Left = 1, Up = 2, Right = 3 };
constexpr int BEAM_DIRS = 4;
// Any number to a way, by its low two bits (as the game reads a level file's).
[[nodiscard]] constexpr BeamDir beamDir(int value) {
	constexpr BeamDir WAYS[BEAM_DIRS] = {BeamDir::Down, BeamDir::Left, BeamDir::Up, BeamDir::Right};
	return WAYS[value & (BEAM_DIRS - 1)];
}
[[nodiscard]] constexpr BeamDir clockwise(BeamDir d) { return beamDir(static_cast<int>(d) + 1); }
[[nodiscard]] constexpr BeamDir opposite(BeamDir d) { return beamDir(static_cast<int>(d) + 2); }
[[nodiscard]] constexpr int beamDx(BeamDir d) { return d == BeamDir::Right ? 1 : (d == BeamDir::Left ? -1 : 0); }
[[nodiscard]] constexpr int beamDy(BeamDir d) { return d == BeamDir::Up ? 1 : (d == BeamDir::Down ? -1 : 0); }
[[nodiscard]] inline BeamDir shaftDir(const Tile& t) { return beamDir(t.attr); }
[[nodiscard]] inline BeamDir mirrorDir(const Tile& t) { return beamDir(t.value); }
inline void turnMirror(Tile& t) { t.value = static_cast<int>(clockwise(mirrorDir(t))); }

// A beam crosses a cell along its middle: a row at half its height, a column at half its width (the drawing's
// BEAM_HEIGHT). How far into a shaped cell a beam going `way` gets before a solid sub-cell on that line (plan
// subcell-physics), in tiles; 1: it goes through.
[[nodiscard]] inline float beamReach(const Tile& t, BeamDir way) {
	if (!hasShape(t))
		return 1.f;
	const bool forward = beamDx(way) + beamDy(way) > 0;
	for (int k = 0; k < SUBCELLS; k++) {
		const int along = forward ? k : SUBCELLS - 1 - k;
		if (beamDy(way) == 0 ? subcellSolid(t, along, SUBCELLS / 2) : subcellSolid(t, SUBCELLS / 2, along))
			return static_cast<float>(k) * SUBCELL_SIZE;
	}
	return 1.f;
}
// The beam going `way` does not get through the cell: a solid sub-cell on its line.
[[nodiscard]] inline bool shapeStopsBeam(const Tile& t, BeamDir way) { return beamReach(t, way) < 1.f; }

// Stops the beam going `way` into the cell: rock, deep water, a gate not fully open, a solid sub-cell on its line. The
// game's rule; the checker's gates follow its mask.
[[nodiscard]] inline bool blocksBeam(const Tile& t, BeamDir way) {
	return isSolidStructure(t.structure) || (t.type == Gate && gateState(t) != GateState::Open) ||
		   shapeStopsBeam(t, way);
}

// One cell a beam lights. in: the way it travels into the cell (a light shaft: its own way); out: the way it leaves
// (a mirror's way); ends: it goes no further (rock or a closed gate next, a mirror turned back at it, or out of the
// level). A mirror pointing the way the beam goes stands edge-on to it (out = in); one pointing back the way it came
// faces it and catches it (out = opposite(in), ends).
struct BeamStep {
	int cell; // index, row * LEVEL_WIDTH + col
	BeamDir in;
	BeamDir out;
	bool ends;
};

// cell (an index) stops the beam going way into it.
using BeamBlock = std::function<bool(int cell, BeamDir way)>;

// The beams of every light shaft in cells (a whole level), as the mirrors stand: each shaft's steps, from the shaft's
// cell on, then the next shaft's. A beam that comes back into a cell the way it went before is a loop: it ends there.
[[nodiscard]] std::vector<BeamStep> traceBeams(const Tile* cells, const BeamBlock& blocked);

// A mirror the checker sets: cell (an index), the way it points.
struct MirrorSetting {
	int cell;
	BeamDir dir;
};

// The cells a beam can light when the player turns the mirrors free(cell) says they can reach (the others stand as
// they are): lit[cell] != 0. With ways not null, ways[cell] holds the mirror settings that light that cell (empty for
// one lit as the mirrors stand), one way of doing it.
[[nodiscard]] std::vector<char> sunlitCells(
	const Tile* cells,
	const BeamBlock& blocked,
	const std::function<bool(int cell)>& free,
	std::vector<std::vector<MirrorSetting>>* ways = nullptr
);

#endif
