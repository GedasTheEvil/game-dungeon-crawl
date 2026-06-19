#ifndef SCREEN_STATE_H
#define SCREEN_STATE_H

#include "../state/cashe.h"

namespace ScreenState {
enum class DrawScreen {
	Menu,
	Inventory,
	Stats,
	Riddle,
	Gameplay,
};

inline DrawScreen GetDrawScreen(const Cashe& c) {
	if (c.ui.menu.show)
		return DrawScreen::Menu;

	if (c.ui.invent->show)
		return DrawScreen::Inventory;

	if (c.ui.Stats->show)
		return DrawScreen::Stats;

	if (c.ui.rid->show)
		return DrawScreen::Riddle;

	return DrawScreen::Gameplay;
}

inline bool ShouldRouteKeyboardToRiddle(const Cashe& c) { return c.ui.rid->show; }

inline bool ShouldBlockKeyboardGameplay(const Cashe& c) { return c.ui.menu.show; }

inline bool ShouldRouteMouseToMenu(const Cashe& c) { return c.ui.menu.show; }

inline bool ShouldRouteMouseToInventory(const Cashe& c) { return c.ui.invent->show; }

inline bool IsGameplayInteractionAllowed(const Cashe& c) { return c.Player->Alive() && !c.IHaveWon; }
} // namespace ScreenState

#endif
