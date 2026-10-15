#ifndef INTERACT_PROMPT_H
#define INTERACT_PROMPT_H

#include "../world/interaction.h"
#include <string>

class Font;

// The interact prompt (plan interact-prompt, layout in ui_layout.h): the interact key's cap and what it does here,
// "[E] Open", in a small panel at the bottom right, in the player HUD's look.
namespace InteractPrompt {
// Once a frame: verb is what the key does in the player's cell, None to hide it (a screen open, travelling, dead,
// won). key: the key cap ("E"), empty when unbound (the verb alone). body: the verb; small: the key. Sets its own
// square-pixel ortho projection, the HUD's, for a resX x resY window. Leaves texturing on and the HUD blend function
// (GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR) set.
void draw(Interaction verb, const std::string& key, int resX, int resY, Font& body, Font& small);
} // namespace InteractPrompt

#endif
