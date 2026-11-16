#ifndef UI_SCREEN_H
#define UI_SCREEN_H

// A tab screen over the game (inventory, map, journal, skills; plan screen-interface): what the frame and the input
// callbacks call on the open one (UIContext::TabScreen). The menu, the riddle and the death screen have their own
// rules (key capture, typing, quick load) in input.cpp.
class UiScreen {
  public:
	virtual void Draw() = 0;
	virtual void KeyPressed(unsigned char key) = 0;
	virtual void SpecialKeyPressed(int key) = 0;
	virtual void MouseFunction(int button, int state, int x, int y) = 0;
	virtual void MouseMotion(int x, int y) = 0; // with or without a button held

  protected:
	UiScreen() = default;
	UiScreen(const UiScreen&) = default;
	UiScreen& operator=(const UiScreen&) = default;
	UiScreen(UiScreen&&) = default;
	UiScreen& operator=(UiScreen&&) = default;
	~UiScreen() = default; // never deleted through the base
};

#endif
