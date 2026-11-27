#ifndef INPUT_H
#define INPUT_H

#include "bindings.h"

const unsigned char KEY_ESCAPE = 27;
const unsigned char KEY_ENTER = 13;
const unsigned char KEY_SPACE = 32;
const unsigned char KEY_TAB = 9;
const unsigned char KEY_DELETE = 127;
const unsigned char KEY_BACKSPACE = 8;

// Key codes the screens read directly and the fixed keys; the gameplay keys are bindings (bindings.h, SpecialKey).
const int SPECIAL_TOGGLE_CARTOON = 1;
const int SPECIAL_TOGGLE_HITBOXES = 3;
const int SPECIAL_QUICK_SAVE = 5; // F5 (plan quick-save)
const int SPECIAL_QUICK_LOAD = 9; // F9
const int SPECIAL_MOVE_LEFT = 100;
const int SPECIAL_MOVE_UP = 101;
const int SPECIAL_MOVE_RIGHT = 102;
const int SPECIAL_MOVE_DOWN = 103;
const int SPECIAL_CAMERA_UP = 104;
const int SPECIAL_CAMERA_DOWN = 105;
const int SPECIAL_CAMERA_LEFT = 106;
const int SPECIAL_CAMERA_RIGHT = 107;
const int SPECIAL_INTERACT = 12;
const int SPECIAL_SHIFT_LEFT = 112;
const int SPECIAL_SHIFT_RIGHT = 113;
const int SPECIAL_KEYPAD_DELETE = 111;

const int MOUSE_LEFT_BUTTON = 0;
const int MOUSE_MIDDLE_BUTTON = 1;
const int MOUSE_RIGHT_BUTTON = 2;
const int MOUSE_WHEEL_UP = 3; // the wheel comes as buttons 3 and 4, a down and an up per notch
const int MOUSE_WHEEL_DOWN = 4;
// A button's state in processMouse and the screens' MouseFunction.
const int MOUSE_DOWN = 0;
const int MOUSE_UP = 1;
const float CAMERA_ROTATE_LIMIT_X = 30.0f;
const float CAMERA_ROTATE_LIMIT_Y = 10.0f;
const float MOUSE_LOOK_SENSITIVITY = 0.08f;

void keyPressed(unsigned char a, int x, int y);
void keyReleased(unsigned char a, int x, int y);

void specialKeyPressed(int a, int x, int y);

void specialKeyReleased(int a, int x, int y);

void processMouse(int a, int b, int c, int d);

void processMouseActiveMotion(int a, int b);

void processMousePassiveMotion(int a, int b);

// The pointer entered or left the window.
void processMouseEntry();

// A gamepad (plan sdl3-gamepad; the SDL events in input.cpp, a scenario's `pad` and `stick` call them too). A button
// (PadButton) went down or up: in the game its binding, on a screen the d-pad is the arrows, South Enter, East Esc;
// Start is Esc everywhere. `caps`: the pad's family, the key caps shown from now on.
void padPressed(int button, Caps caps);
void padReleased(int button);
// A stick moved to (x, y), -1..1 each, y down: the left one walks and climbs (on a screen: the arrows), the right one
// looks (stepPadLook).
void padStickMoved(bool right, float x, float y, Caps caps);
// Once a tick in the game: the right stick turns the look.
void stepPadLook();
// The pad went away: every button and stick it held lets go.
void padLetGo();

#endif
