#ifndef BOSS_BAR_H
#define BOSS_BAR_H

class Font;
struct StatusList;

// HUD bar of the boss in a fight: its name over a long health bar at the bottom centre (the status message owns
// the top centre).
namespace BossBar {
// ratio: health left, 0..1; poisoned: the bar is green, like the player's. statuses: the boss's conditions
// (monsterStatuses, plan monster-status-icons), a row of cards at the player's card size over the bar's left end;
// icons, cards: the icon atlas and the card backgrounds. Sets its own square-pixel ortho projection (100 high) for a
// resX x resY window. Leaves texturing on and the HUD blend function (GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR) set.
struct Look {
	int resX = 0, resY = 0;
	Font* font = nullptr;
	int icons = 0, cards = 0;
};
void draw(const char* name, float ratio, bool poisoned, const StatusList& statuses, const Look& look);
} // namespace BossBar

#endif
