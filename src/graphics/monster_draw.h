#ifndef MONSTER_DRAW_H
#define MONSTER_DRAW_H

#include "character_model.h"
#include "../sim/monster.h"
#include "texture_registry.h"

// A monster as its last Animate left it, with the frame origin at its spawn tile: the model of its type (Assets), its
// blood, and its health bar once it is alerted (not a boss's: that one is on the HUD).
// texture >= 0: the body only (no health bar, no blood) with that texture on its UVs (a contour pass,
// Dungeon::drawContour).
void DrawMonster(const Monster& mon, const CharacterModel& model, const TextureRegistry& textures, int texture = -1);
// An entombed monster's model lying in its coffin, still (the idle clip's first frame), in DrawMonster's frame of a
// monster on its spawn tile: the decoy of plan placed-decor, which never wakes.
void DrawEntombed(const MonsterType& type, const CharacterModel& model);

#endif
