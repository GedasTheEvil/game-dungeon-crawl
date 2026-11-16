#ifndef INPUT_ACTIONS_H
#define INPUT_ACTIONS_H

#include "../level/input.h"
#include "../level/bindings.h"
#include "../state/gameplay_commands.h"
#include <string>

// The cursor hidden over the game, shown on the menu and every other screen.
void followCursor();

// The gameplay action a binding runs; None for the ones handled outside it (sprint, look, the screens).
GameplayAction gameplayActionOf(BindAction action);

#endif
