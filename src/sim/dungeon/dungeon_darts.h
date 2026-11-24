#ifndef DUNGEON_DARTS_H
#define DUNGEON_DARTS_H

#include <vector>

// Dart traps (dungeon_darts.cpp, plan poison-dart-trap): a pressed plate makes the holes in the back wall above it
// shoot a volley of poison darts out towards the camera, across the corridor.
struct DartTraps {
	struct Plate {
		int cell; // map index
		int startMs; // GameClock time of the click
	};
	struct Volley {
		int startMs; // the plate's click
		int col, row;
		int fired = 0; // darts out so far
	};
	struct Dart {
		float x, y; // map units: its hole (DART_HOLE_X), at DART_HEIGHT
		float depth = 0.f; // tiles out of the back wall: 0 the wall, 1 the front of the corridor
		int lastMs; // GameClock time of the last update
		bool passed = false; // crossed the walking line (DART_HIT_DEPTH) without a hit
	};
	std::vector<Plate> pressed; // down until it re-arms
	std::vector<Volley> volleys;
	std::vector<Dart> darts;
};

#endif
