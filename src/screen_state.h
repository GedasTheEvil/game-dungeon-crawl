#ifndef SCREEN_STATE_H
#define SCREEN_STATE_H

#include "cashe.h"

namespace ScreenState {
enum class DrawScreen {
	Menu,
	Inventory,
	Stats,
	Riddle,
	Gameplay,
};

inline DrawScreen GetDrawScreen(const Cashe& c) {
	if (c.menu.show)
		return DrawScreen::Menu;

	if (c.invent->show)
		return DrawScreen::Inventory;

	if (c.Stats->show)
		return DrawScreen::Stats;

	if (c.rid->show)
		return DrawScreen::Riddle;

	return DrawScreen::Gameplay;
}

inline bool ShouldRouteKeyboardToRiddle(const Cashe& c) { return c.rid->show; }

inline bool ShouldBlockKeyboardGameplay(const Cashe& c) { return c.menu.show; }

inline bool ShouldRouteMouseToMenu(const Cashe& c) { return c.menu.show; }

inline bool ShouldRouteMouseToInventory(const Cashe& c) { return c.invent->show; }

inline bool IsGameplayInteractionAllowed(const Cashe& c) { return c.Player->Alive() && !c.IHaveWon; }
} // namespace ScreenState

#endif
