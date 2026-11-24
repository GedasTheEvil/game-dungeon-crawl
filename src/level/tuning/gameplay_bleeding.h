#ifndef GAMEPLAY_BLEEDING_H
#define GAMEPLAY_BLEEDING_H

// Bleeding (plan bleeding, level/rules/bleeding.h): the base chance of a bleed per source, before the victim's cut rate
// and its trap share (bleedChancePercent). A cutting bite: BLEED_BITE_PERCENT x its cut share (a rat's 30%: 9%), a
// boss's BOSS_BLEED_BITE_PERCENT. The spikes (floor, wall, ceiling) a weak bleed, the death traps (floor, ceiling) a
// medium one, once per entry.
constexpr int BOSS_BLEED_BITE_PERCENT = 45;

#endif
