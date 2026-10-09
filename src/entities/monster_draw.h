#ifndef MONSTER_DRAW_H
#define MONSTER_DRAW_H

#include "character_model.h"
#include "monster.h"
#include "../graphics/texture_registry.h"

// A monster as its last Animate left it, with the frame origin at its spawn tile: the model of its type (Assets), its
// blood, and its health bar once it is alerted (not a boss's: that one is on the HUD).
void DrawMonster(const Monster& mon, const CharacterModel& model, const TextureRegistry& textures);

#endif
