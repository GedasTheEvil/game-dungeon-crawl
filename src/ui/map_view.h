#ifndef MAP_VIEW_H
#define MAP_VIEW_H

#include "map_zoom.h"

// The archaeologist's draft map ([M]): a pencil sketch on papyrus of the cells explored on this level
// (Dungeon::Explored). Walls are hatched and outlined, ladders, doors, chests, traps and mechanisms get a symbol,
// keys, gates and levers are drawn in their lock colour. A legend slip beside the sheet names the symbols drawn so far.
// The wheel and + / - zoom, a drag and the arrows pan (plan map-symbols-and-zoom). The game is paused while it is
// open.
class DraftMap {
  public:
	void Draw();
	void KeyPressed(unsigned char key);
	void SpecialKeyPressed(int key);
	void MouseFunction(int button, int state, int x, int y);
	void MouseMotion(int x, int y);

  private:
	MapZoom zoom;
	MapFit fit{}; // the last frame's, for the input between frames
	float canvasW = 0.f;
	int level = -1; // the level the zoom is for: a new one starts at 1x
	bool dragging = false;
	float dragX = 0.f, dragY = 0.f;

	void toCanvas(int x, int y, float& cx, float& cy) const;
};

#endif
