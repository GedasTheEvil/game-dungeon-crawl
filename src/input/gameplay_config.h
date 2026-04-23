#ifndef GAMEPLAY_CONFIG_H
#define GAMEPLAY_CONFIG_H

constexpr float PLAYER_MOVE_STEP = 0.025f;
constexpr float PLAYER_FORWARD_MOVE_STEP = 0.0225f;

constexpr float JUMP_FORWARD_SPEED = 0.054f;
constexpr float JUMP_INITIAL_VELOCITY = 0.085f;
constexpr float JUMP_GRAVITY_STEP = 0.01f;

constexpr int JUMP_TIMER_MS = 5000;
constexpr int JUMP_UP_TIMER_MS = 4000;
constexpr int JUMP_TICK_MS = 40;
constexpr int FALL_TICK_MS = 40;

constexpr float FALL_STEP = 0.03f;
constexpr float FALL_START_THRESHOLD = 0.03f;

constexpr int TRAP_HURT_INTERVAL_MS = 100;
constexpr float TRAP_HITBOX_X_SCALE = 0.02f;
constexpr float TRAP_HITBOX_Y_SCALE = 0.006f;

constexpr float MONSTER_SEEK_STEP = 0.0042f;

#endif
