#ifndef SCREEN_TABS_H
#define SCREEN_TABS_H

#include "screen.h"

// The tab strip of the inventory, the draft map and the journal (ui::screenTabs): a click on a tab, or its key
// (I, M, J by default, input.cpp), switches straight to that screen. The open screen's own key still closes it to the
// game.
struct ScreenTabsState {
	int hovered = -1;
	int pressed = -1; // the mouse went down on this tab; the switch happens when it comes up on the same one
};

namespace ScreenTabs {
[[nodiscard]] bool Has(Screen s); // the screen shows the strip
// Over the open screen, in the 160 x 100 canvas; the screen's projection need not be that canvas.
void Draw();
// True when the click was on the strip (the screen does not get it).
bool Mouse(int button, int state, int x, int y);
void Motion(int x, int y);
} // namespace ScreenTabs

#endif
