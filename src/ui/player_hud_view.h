#ifndef PLAYER_HUD_VIEW_H
#define PLAYER_HUD_VIEW_H

#include "player_hud.h"

// The player HUD's values now: health, stamina, XP, keys and the quick slots, from the player, the dungeon and the
// inventory. PlayerHud::draw only draws what it is given.
[[nodiscard]] PlayerHud::View playerHudView();

#endif
