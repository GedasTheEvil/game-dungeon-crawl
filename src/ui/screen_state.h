#ifndef SCREEN_STATE_H
#define SCREEN_STATE_H

#include "../state/game_state.h"

namespace ScreenState {
inline Screen GetDrawScreen(const GameState& c) { return c.ui.screen; }
inline bool IsOpen(const GameState& c, Screen s) { return c.ui.screen == s; }

inline bool ShouldRouteKeyboardToRiddle(const GameState& c) { return IsOpen(c, Screen::Riddle); }

inline bool ShouldBlockKeyboardGameplay(const GameState& c) { return IsOpen(c, Screen::Menu); }

inline bool ShouldRouteMouseToMenu(const GameState& c) { return IsOpen(c, Screen::Menu); }

inline bool ShouldRouteMouseToInventory(const GameState& c) { return IsOpen(c, Screen::Inventory); }

inline bool IsGameplayInteractionAllowed(const GameState& c) { return c.player->Alive() && !c.dungeon.Won(); }
} // namespace ScreenState

#endif
