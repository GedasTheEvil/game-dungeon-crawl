#ifndef GAMEPLAY_PLAYER_H
#define GAMEPLAY_PLAYER_H

#include "gameplay_shared.h"

constexpr float PLAYER_SCALE = 15.f; // the archeologist model's scale
// Walls, gates and solid sub-cells stop the player at the hitbox (Player::HalfWidth, about 0.07 tiles, from the
// model). This is a bound over it for compile-time margins: the time to walk through spikes, the level checker.
constexpr float PLAYER_BODY_HALF_WIDTH = PLAYER_SCALE / 60.f;
// Climbing up needs ladder this far above the player's feet (their height, roughly), so they stop with the head
// below the top rung.
constexpr float PLAYER_CLIMB_HEADROOM = PLAYER_SCALE / 40.f;
// The player's body against the solid sub-cells of a shaped cell (plan subcell-physics): its height, about 0.375 tiles.
constexpr float PLAYER_BODY_HEIGHT = PLAYER_CLIMB_HEADROOM;
// Crouch (plan crouch): the body and the hitbox are CROUCH_HEIGHT_FACTOR of the standing ones (0.206 tiles: under a
// crawl space a quarter high; the hitbox's top under SHOT_CHEST, so a shot aimed at the standing chest flies over). The
// walk is CROUCH_WALK_FACTOR of the walk speed; the stand-to-crouch clip takes CROUCH_TRANSITION_MS either way.
constexpr float CROUCH_HEIGHT_FACTOR = 0.55f;
constexpr float CROUCH_WALK_FACTOR = 1.f / 3.f;
constexpr int CROUCH_TRANSITION_MS = 200;
// Dragging (plan mirror-drag): the player is crouched, the walk DRAG_WALK_FACTOR of the crouched one (a quarter of the
// walk). The drag key takes hold of a thing in the player's cell or the next one they face, its centre at most
// DRAG_REACH tiles from theirs; the player steps to DRAG_HOLD tiles from it, the hands on it, and keeps that while
// dragging.
constexpr float DRAG_WALK_FACTOR = 0.75f;
constexpr float DRAG_REACH = 1.05f;
constexpr float DRAG_HOLD = 0.25f;
// Update() runs once a tick (game.cpp's timer, the scenario tick). A held walk key moves the player once a tick
// (stepHeldWalk), so the walk speed is WALK_SPEED on every machine, not the key repeat rate.
constexpr float PLAYER_MOVE_STEP = WALK_SPEED * static_cast<float>(UPDATE_TICK_MS) / 1000.f;
constexpr float PLAYER_FORWARD_MOVE_STEP = 0.9f * PLAYER_MOVE_STEP; // climbing up

// Map x within a cell at which the drawn player (always at the screen centre) is in front of the ladder:
// tile i is drawn from 40 * (i - mapX) - 2 and the ladder stands at its middle (DungeonView::Draw). Climbing pulls the
// player there.
constexpr float LADDER_GRIP_X = 0.45f;
// The climb clip's phase per tile moved along the rungs, as a share of a tile climbed: the hands move on a sideways
// climb too (Dungeon::ClimbPhase, Monster::showClimb).
constexpr float CLIMB_SIDE_RATE = 0.5f;

constexpr float JUMP_FORWARD_SPEED = 0.054f;
constexpr float JUMP_INITIAL_VELOCITY = 0.085f;
constexpr float JUMP_GRAVITY_STEP = 0.01f;
constexpr int JUMP_STAMINA_COST = 20;

constexpr float FALL_GRAVITY_STEP = JUMP_GRAVITY_STEP;
// < 1 tile, so one tick never skips a floor; a step's floor inside a cell neither: the landing looks a cell down from
// where the tick starts (Dungeon::floorBelow).
constexpr float FALL_MAX_STEP = 0.3f;
constexpr float FALL_START_THRESHOLD = 0.03f;

#endif
