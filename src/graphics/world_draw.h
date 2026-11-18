#ifndef WORLD_DRAW_H
#define WORLD_DRAW_H

// What the world's drawing (dungeon_render*.cpp) draws with on the GL core path (plan gl-core-world): gMv and gProj,
// GlState, and the world's batch.
#include "../render/gl_legacy.h"
#include "../render/gl_state.h"
#include "../render/lighting.h"
#include "../render/matrix_stack.h"

// The world's batch (Lighting::batch): immediate mode lit by the world shader, its current colour also a model's.
inline Batch& world() { return Lighting::batch(); }

#endif
