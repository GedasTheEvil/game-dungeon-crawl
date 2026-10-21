#ifndef ZONE_BANNER_H
#define ZONE_BANNER_H

#include "../world/campaign.h"

class Font;

// The banner over the game when a campaign level starts (plan zone-looks, levelBanner): on a zone's first level a
// large one with the hour and its gate, the zone and its guardian; on any other the level and its zone, smaller.
// A framed panel in the upper third, under the status box; it fades in and out.
namespace ZoneBanner {
constexpr int ZONE_MS = 6500;
constexpr int LEVEL_MS = 3500;
constexpr int FADE_MS = 900;

// ageMs since the level started: 0 .. 1, 0 once it is over.
[[nodiscard]] float Alpha(int ageMs, bool zone);
// Sets its own square-pixel ortho projection (100 high). Leaves texturing on and the HUD blend function set.
void draw(const LevelBanner& banner, float alpha, int resX, int resY, Font& title, Font& body);
} // namespace ZoneBanner

#endif
