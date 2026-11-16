#ifndef SCREEN_STATE_H
#define SCREEN_STATE_H

#include "../state/game_state.h"

namespace ScreenState {
inline bool IsGameplayInteractionAllowed(const GameState& c) {
	return c.player->Alive() && !c.dungeon.Won() && !c.dungeon.Travelling();
}
} // namespace ScreenState

#endif
