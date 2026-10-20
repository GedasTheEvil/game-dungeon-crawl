#ifndef DEATH_SCREEN_H
#define DEATH_SCREEN_H

#include "../graphics/font.h"
#include <cstdint>

/// @file death_screen.h
/// The death screen (plan autosave): once the death clip has played, a panel over the dimmed game with two buttons,
/// Retry from Autosave (greyed out without an autosave) and Main Menu. A screen of its own (Screen::Death): no ticks,
/// no other screen opens over it.

class DeathScreen {
  public:
	// Each update: counts the death once (GameState::CountDeath), opens the screen DEATH_SCREEN_DELAY_MS later (from
	// the game, not over a menu), resets when the player lives again (a load, a new game).
	void Follow();
	void KeyPressed(unsigned char key); // Enter picks, Esc = Main Menu
	void SpecialKeyPressed(int key); // the arrows move the focus
	void MouseFunction(int button, int state, int x, int y);
	void MouseMotion(int x, int y);
	void Draw(int resX, int resY); // over the game, drawn just before

  private:
	enum class State : std::uint8_t { Alive, Dying, Shown };
	static constexpr int NONE = -1;
	static constexpr int RETRY = 0;
	static constexpr int MAIN_MENU = 1;
	State state = State::Alive;
	int diedMs = 0;
	bool retryEnabled = false; // an autosave was there when the screen opened
	int focus = MAIN_MENU; // the keys' and the mouse's button
	int pressed = NONE; // mouse went down here; the action runs when it comes up on the same button
	bool fontsLoaded = false;
	Font title, heading, body, small;

	[[nodiscard]] int buttonAt(int x, int y) const;
	void activate(int button);
};

#endif
