// Cut-scenes (plan cinematic-effects, plan dungeon-subsystems): the camera's way to the gates a lever or a boss's death
// opened off the screen and back. The Dungeon starts the gates' motion and stands the world still meanwhile
// (cut_scene_dungeon.cpp).
#ifndef CUT_SCENE_H
#define CUT_SCENE_H
#include "../../level/view_window.h"
#include <vector>

// A cut-scene's camera this tick (plan cinematic-effects): how far it is from the player in tiles (x right, y up), and
// how far the cut-scene look (letterbox bars, tint, vignette) is in, 0..1.
struct CutShot {
	float dx = 0.f;
	float dy = 0.f;
	float look = 0.f;
};

class CutScene {
  public:
	// A gate the camera shows (map index) and when it comes to it (GameClock): the gate starts to open then.
	struct Stop {
		int cell;
		int arrivalMs;
	};
	void SetEnabled(bool on) { enabled = on; } // Options > Graphics > Cut-scenes; off: remote gates open unseen
	// The cut-scene for the gates just opened (cells): the nearest CUTSCENE_MAX_STOPS outside view, from the player at
	// (x, y). Empty (none started): off, one already playing, or all on the screen.
	std::vector<Stop> Start(const std::vector<int>& opened, ViewWindow view, float x, float y);
	// The camera this tick, for the player at (x, y). False: the scene has run its length (End it).
	bool Update(float x, float y);
	void End(); // back to the player at once
	[[nodiscard]] bool Playing() const { return !stops.empty(); }
	[[nodiscard]] CutShot Shot() const { return shot; }

  private:
	std::vector<int> stops; // gate cells (map index), in the order shown; empty: none playing
	int startMs = 0;
	bool enabled = false;
	CutShot shot; // this tick's camera, set by Update
	[[nodiscard]] int lengthMs() const;
};

#endif
