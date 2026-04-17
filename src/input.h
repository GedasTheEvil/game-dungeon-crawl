#ifndef DInputH
#define DInputH

#include "gameplay_config.h"

const unsigned char KEY_ESCAPE = 27;
const unsigned char KEY_ENTER = 13;
const unsigned char KEY_SPACE = 32;
const unsigned char KEY_MOVE_LEFT = 'a';
const unsigned char KEY_MOVE_RIGHT = 'd';
const unsigned char KEY_MOVE_DOWN = 's';
const unsigned char KEY_MOVE_UP = 'w';
const unsigned char KEY_INVENTORY = 'i';
const unsigned char KEY_STATS = 'o';

const int SPECIAL_TOGGLE_CARTOON = 1;
const int SPECIAL_TOGGLE_ORIGINAL_MODEL = 2;
const int SPECIAL_MOVE_LEFT = 100;
const int SPECIAL_MOVE_UP = 101;
const int SPECIAL_MOVE_RIGHT = 102;
const int SPECIAL_MOVE_DOWN = 103;
const int SPECIAL_CAMERA_UP = 104;
const int SPECIAL_CAMERA_DOWN = 105;
const int SPECIAL_CAMERA_LEFT = 106;
const int SPECIAL_CAMERA_RIGHT = 107;
const int SPECIAL_INTERACT = 12;

const int MOUSE_LEFT_BUTTON = 0;
const int MOUSE_MIDDLE_BUTTON = 1;
const int MOUSE_RIGHT_BUTTON = 2;
const float CAMERA_ROTATE_STEP = 2.5f;
const float CAMERA_ROTATE_LIMIT_X = 30.0f;
const float CAMERA_ROTATE_LIMIT_Y = 10.0f;
const float MOUSE_LOOK_SENSITIVITY = 0.08f;

void Load();

void Draw();

void Idle();

void keyPressed(unsigned char a, int x, int y);

void specialKeyPressed(int a, int x, int y);

void processMouse(int a, int b, int c, int d);

void processMouseActiveMotion(int a, int b);

void processMousePassiveMotion(int a, int b);

void processMouseEntry(int a);

#endif
