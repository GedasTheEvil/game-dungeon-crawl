#ifndef ENDING_SCREEN_H
#define ENDING_SCREEN_H

#include "../render/font.h"
#include "../level/bindings.h"
#include "../level/pointer.h"
#include "screen_world.h"

/// @file ending_screen.h
/// The ending choice (plan ending-choice): using the ankh asks first, a panel over the dimmed game with two buttons,
/// Take the ankh (life without end) and Renounce the gods (the ankh broken). A screen of its own (Screen::Ending): no
/// ticks meanwhile. Esc goes back to the game with nothing chosen; a pick is final (Dungeon::Win), the win flow follows
/// (WinEntry).

struct UIContext;

class EndingScreen {
  public:
	EndingScreen(const ScreenWorld& world, UIContext& ui) : world(world), ui(ui) {}
	void Open(); // over the game: the focus on Take
	void KeyPressed(unsigned char key); // Enter picks, Esc back to the game
	void SpecialKeyPressed(int key); // the arrows (and the bound move keys) move the focus
	void MouseFunction(int button, int state, int x, int y);
	void MouseMotion(int x, int y);
	void Draw(int resX, int resY); // over the game, drawn just before

  private:
	const ScreenWorld& world;
	UIContext& ui;
	static constexpr int NONE = -1;
	static constexpr int TAKE = 0;
	static constexpr int RENOUNCE = 1;
	ui::Pointer<int> pointer{NONE}; // hovered: the keys' and the mouse's button, kept when the mouse leaves it
	int fontGeneration = -1; // ensureScreenFonts
	Font title, heading, body, small;

	[[nodiscard]] int buttonAt(int x, int y) const;
	void choose(int button);
	void navigate(NavKey key);
};

#endif
