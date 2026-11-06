#ifndef STATUS_BOX_H
#define STATUS_BOX_H

#include "../level/message_log.h"
#include <vector>

class Font;

// The gameplay status messages ("Gained 20 XP", "Found: Sword", plan message-log): framed panels at the top centre,
// each sized to its text, one line per '\n', stacked down, oldest at the top. Each fades in, then out at the end of
// its time; good news and dangers in their colours.
namespace StatusBox {
// The lines MessageLog::Visible gives at GameClock time now. Sets its own square-pixel ortho projection (100 high)
// for a resX x resY window. Leaves texturing on and the HUD blend function (GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR) set.
// Returns the free top under the stack (TOP when nothing shows), on that canvas: the level banner goes there.
float draw(const std::vector<const StatusMessage*>& shown, int now, int resX, int resY, Font& font);
} // namespace StatusBox

#endif
