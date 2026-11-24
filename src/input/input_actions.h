#ifndef INPUT_ACTIONS_H
#define INPUT_ACTIONS_H

#include "../level/input/keys.h"
#include "../level/input/bindings.h"
#include "../state/gameplay_commands.h"
#include <string>

// The pointer held over the game (hidden, relative mode: Window::holdPointer), free on the menu and every other
// screen.
void followCursor();

// A key held down sends no repeats while true (a cut-scene: a key held from before does not skip it).
void ignoreKeyRepeat(bool ignore);

// The gameplay action a binding runs; None for the ones handled outside it (sprint, look, the screens).
GameplayAction gameplayActionOf(BindAction action);

#endif
