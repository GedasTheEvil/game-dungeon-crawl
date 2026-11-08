#ifndef FIRST_HINTS_H
#define FIRST_HINTS_H

// The first hints (plan controls-help): once a gameplay tick, a hint the first time its thing is possible in the run
// (a gap ahead, a ladder, a monster coming, ...), as a status line naming the bound key (hintText). Each shows once a
// run (RunStats::hints, in the save), one at a time; a thing the player already did needs no hint and is marked shown.
// Only in a campaign run (scenario `hints on`: any) and with the How to Play page's switch on (Settings::Gameplay).
void updateFirstHints();

#endif
