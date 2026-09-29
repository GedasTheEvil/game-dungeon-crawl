#ifndef STATUS_BOX_H
#define STATUS_BOX_H

#include <string>

class Font;

// The gameplay status message ("Gained 20 XP", "Found: Sword"): a framed panel at the top centre, sized to the
// text, one line per '\n'. Fades in, then out at the end of its time.
namespace StatusBox {
// ageMs since the message was shown, out of shownMs. Sets its own square-pixel ortho projection (100 high) for a
// resX x resY window. Leaves texturing on and the HUD blend function (GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR) set.
void draw(const std::string& message, int ageMs, int shownMs, int resX, int resY, Font& font);
} // namespace StatusBox

#endif
