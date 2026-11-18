#ifndef INPUT_EVENTS_H
#define INPUT_EVENTS_H

union SDL_Event;

// SDL's key and mouse events to the game's input (plan sdl3-window-input): keys as the game's codes (Keymap) to
// keyPressed / specialKeyPressed and their releases, buttons and the wheel to processMouse, motions in pixels
// (density: SDL_GetWindowPixelDensity) to the motion functions, or to the look while the pointer is held. A scenario
// calls those functions itself (level/input.h).
void handleInputEvent(const SDL_Event& event, float density);

#endif
