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

	if (c.ui.invent->show)
		return DrawScreen::Inventory;

	if (c.ui.rid->show)
		return DrawScreen::Riddle;

	if (c.ui.map.show)
		return DrawScreen::Map;

	return DrawScreen::Gameplay;
}

inline bool ShouldRouteKeyboardToRiddle(const GameState& c) { return c.ui.rid->show; }

inline bool ShouldBlockKeyboardGameplay(const GameState& c) { return c.ui.menu.show; }

inline bool ShouldRouteMouseToMenu(const GameState& c) { return c.ui.menu.show; }

inline bool ShouldRouteMouseToInventory(const GameState& c) { return c.ui.invent->show; }

inline bool IsGameplayInteractionAllowed(const GameState& c) { return c.Player->Alive() && !c.IHaveWon; }
} // namespace ScreenState

#endif
