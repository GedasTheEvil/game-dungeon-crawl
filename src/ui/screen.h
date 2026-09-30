#ifndef SCREEN_H
#define SCREEN_H

#include <cstdint>

// The screen shown: the game, or one UI screen over it. One at a time (GameState::ui.screen); the input decides
// which keys switch between them (input.cpp).
enum class Screen : std::uint8_t {
	Menu, // the main menu at start, or the in-game menu (MainMenu::inGame)
	Inventory,
	Map,
	Riddle,
	Gameplay,
};

#endif
