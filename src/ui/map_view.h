#ifndef MAP_VIEW_H
#define MAP_VIEW_H

// The archaeologist's draft map ([M]): a pencil sketch on papyrus of the cells explored on this level
// (Dungeon::Explored). Walls are hatched and outlined, ladders, doors, chests, traps and mechanisms get a symbol,
// keys, gates and levers are drawn in their lock colour. The game is paused while it is open.
class DraftMap {
  public:
	bool show = false;
	void Draw();
};

#endif
