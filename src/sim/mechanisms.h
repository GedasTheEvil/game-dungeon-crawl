// The level's mechanisms' live state (plan dungeon-subsystems): the gates in motion, the pressure plates' weights, the
// sunlight mirrors' beams. A gate's, plate's, mirror's and disc's lasting state is in its tile; the Dungeon ties them
// to the player, the monsters and the world's events (dungeon_mechanisms.cpp, dungeon_plates.cpp, dungeon_sun.cpp).
#ifndef MECHANISMS_H
#define MECHANISMS_H
#include "../level/level.h"
#include "../level/sun_beam.h"
#include <array>
#include <functional>
#include <vector>

// Something in a cell (map index) that moved from startMs (GameClock): a gate, a rock fall, a sun disc's flare.
struct CellMotion {
	int cell;
	int startMs;
};

// The gates on the move (GATE_OPEN_MS): an Opening gate's tile is in `opening`, a Closing one's (a plate gate) in
// `closing`. Each method takes the level's cells.
class GateMotions {
  public:
	void Clear();
	// A closed gate in cell starts to open at startMs; any other cell is left as it is.
	void StartOpening(Tile* map, int cell, int startMs);
	void OpenPlate(Tile* map, int cell, int now); // from closed, or back up from closing (plan pressure-plate)
	void ClosePlate(Tile* map, int cell, int now); // from open, or back down from opening
	void FinishOpened(Tile* map); // the gates whose motion has run are open
	void FinishClosed(Tile* map); // ... closed
	void DelayOpening(int cell, int startMs); // a cut-scene: the gate starts to open as the camera comes to it
	void HurryOpening(int now); // a gate whose motion has not started yet starts now
	// How far the gate in cell (its state) is up, 0 down to 1 up inside the ceiling, eased.
	[[nodiscard]] float Lift(int cell, GateState state) const;

  private:
	std::vector<CellMotion> opening; // c = 2 while in here, then 1 (open)
	std::vector<CellMotion> closing; // c = 3 while in here, then 0 (closed)
};

// Pressure plates (plan pressure-plate): which carry their weight, and the colours whose plate gates opened once.
class PressurePlates {
  public:
	// The plates of each colour (attr) the level has, and those whose every plate carries its weight.
	struct Weighed {
		std::array<bool, LOCK_COLOUR_COUNT + 1> plates{};
		std::array<bool, LOCK_COLOUR_COUNT + 1> held{};
		bool clicked = false; // a plate went down or up (not on the first weighing after a load)
		bool settled = false; // weighed before since the load: the gates' sound and message play
	};
	void Reset(); // a level loaded
	// Weighs every plate of the level (cells) with weightOn(col, row).
	Weighed Weigh(const Tile* map, const std::function<int(int, int)>& weightOn);
	[[nodiscard]] bool Down(int cell) const { return down[static_cast<size_t>(cell)] != 0; } // drawn sunk
	// The plate gates of lock colour opened on this visit (the status line and the cut-scene come once).
	[[nodiscard]] bool OpenedOnce(int lock) const { return (opened & (1 << (lock - 1))) != 0; }
	void MarkOpened(int lock) { opened |= 1 << (lock - 1); }

  private:
	std::vector<char> down = std::vector<char>(LEVEL_CELL_COUNT, 0); // a plate carries its weight
	bool settled = false; // the first weighing after a load sets `down` without a click
	int opened = 0; // bit (colour - 1)
};

// Sunlight mirrors (plan sunlight-mirrors): the beams as the mirrors and gates stand, the cells they light; a turned
// mirror swings from its old angle, a sun disc just lit flares.
class SunMirrors {
  public:
	void Clear(); // the swings and flares (a load)
	void Trace(const Tile* map); // the beams and the lit cells as the level (cells) stands
	void Expire(int now); // the swings and flares that have run
	[[nodiscard]] const std::vector<BeamStep>& Beams() const { return beams; }
	[[nodiscard]] bool Sunlit(int cell) const { return lit[static_cast<size_t>(cell)] != 0; }
	void LightDisc(int cell, int now) { flares.push_back({cell, now}); } // a sun disc just lit: it flares
	// 0..1: how much of the flare of the sun disc in cell is left.
	[[nodiscard]] float Flare(int cell) const;
	// A mirror turned in cell: it swings from fromDeg (its angle before the turn).
	void Swing(int cell, float fromDeg);
	void MoveSwing(int from, int to); // a dragged mirror's swing goes with it
	// Degrees round the depth axis, counter-clockwise as the camera sees it, from up: the face of the mirror in cell
	// (map) as it reflects the beam that falls on it (from above when none does); Angle swings after a turn.
	[[nodiscard]] float FaceDeg(const Tile* map, int cell) const;
	[[nodiscard]] float Angle(const Tile* map, int cell) const;

  private:
	struct MirrorSwing {
		int cell;
		int startMs;
		float fromDeg;
	};
	std::vector<BeamStep> beams;
	std::vector<char> lit = std::vector<char>(LEVEL_CELL_COUNT, 0);
	std::vector<MirrorSwing> swings;
	std::vector<CellMotion> flares;
};

#endif
