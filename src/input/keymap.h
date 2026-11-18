#ifndef KEYMAP_H
#define KEYMAP_H

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_scancode.h>
#include <optional>
#include "../level/bindings.h"

// SDL's keys as the game's (plan sdl3-window-input): an ASCII character or a special key code (input.h, bindings.cpp;
// the numbers GLUT had, so the [controls] names and the scenario keys stay as they were).
namespace Keymap {

// The key at this place on the keyboard, read as on a US layout: the bindings and the menus go by where a key is, not
// by what the layout prints on it (W A S D stay a cross on AZERTY, the number row stays 1-5 on Lithuanian). numLock:
// the keypad gives digits, else its arrows, Home, End, PgUp, PgDn, Ins, Del and Begin (as GLUT did). None: a key the
// game has no code for.
[[nodiscard]] std::optional<InputKey> physical(SDL_Scancode scancode, bool numLock);

// The character a key types with the layout and Shift (the layout's keycode, SDL_GetKeyFromScancode), for the text
// fields (the riddle's answer, the Hall of Records name). 0: not printable ASCII and not Enter, Esc, Tab, Backspace or
// Delete.
[[nodiscard]] unsigned char typed(SDL_Keycode key);

} // namespace Keymap

#endif
