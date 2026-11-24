#ifndef STATUS_CARD_H
#define STATUS_CARD_H

#include "../level/ui/status_list.h"
#include "../render/ui_draw.h"

// One status card (plan curse-blessing-cards): the player's HUD draws a row of them, a monster's health bar another
// (plan monster-status-icons). Here, not in ui/hud/, so the world's drawing needs no screen (plan src-topic-subdirs).
namespace StatusCard {
// The card's size on the HUD canvas, the size its parts are measured on.
constexpr float HUD_SIZE = 6.f;

// Fills r, in the canvas or world space set up: the shadow, the kind's card round r, the icon tinted, the pips if
// asked (too small to read on a monster's row). icons: the HUD icon atlas; cards: the card backgrounds (one cell per
// StatusKind). Leaves the alpha blend on.
void draw(const Status& s, const ui::Rect& r, int icons, int cards, bool pips);
} // namespace StatusCard

#endif
