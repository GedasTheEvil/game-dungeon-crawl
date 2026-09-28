#ifndef SCREEN_STATE_H
#define SCREEN_STATE_H

#include "../state/game_state.h"

namespace ScreenState {
enum class DrawScreen : unsigned char {
	Menu,
	Inventory,
	Map,
	Riddle,
	Gameplay,
};

inline DrawScreen GetDrawScreen(const GameState& c) {
	if (c.ui.menu.show)
		return DrawScreen::Menu;

	if (c.ui.inventory->show)
		return DrawScreen::Inventory;

	if (c.ui.riddle->show)
		return DrawScreen::Riddle;

	if (c.ui.map.show)
		return DrawScreen::Map;

	return DrawScreen::Gameplay;
}

inline bool ShouldRouteKeyboardToRiddle(const GameState& c) { return c.ui.riddle->show; }

inline bool ShouldBlockKeyboardGameplay(const GameState& c) { return c.ui.menu.show; }

inline bool ShouldRouteMouseToMenu(const GameState& c) { return c.ui.menu.show; }

inline bool ShouldRouteMouseToInventory(const GameState& c) { return c.ui.inventory->show; }

inline bool IsGameplayInteractionAllowed(const GameState& c) { return c.Player->Alive() && !c.hasWon; }
} // namespace ScreenState

#endif
