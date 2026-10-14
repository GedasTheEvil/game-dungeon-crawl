#ifndef PATHS_H
#define PATHS_H
#include <string>

// The repo's data roots (plan directory-layout), relative to the repo root, the working directory of the game and
// the tools. Macros, so a path is one literal: MEDIA_DIR "textures/items/%s.png".
// media/: what the game shows and plays (textures, fonts under textures/, models, sounds).
#define MEDIA_DIR "media/"
// content/: what the game reads as data (levels, inscriptions, riddles).
#define CONTENT_DIR "content/"

namespace Paths {
// The repo root: the parent of the bin/ the running executable is in. Empty when the executable is not in a bin/.
std::string rootDir();
// Makes the repo root the working directory, so a program started from elsewhere (cd /tmp && ~/repo/bin/game) finds
// media/, content/, saves/ and game.log as from the root. Call first in main, before anything opens a file.
void enterRootDir();
// A file named on the command line, relative to where the program was started: the same path once enterRootDir()
// moved there, made absolute when it names a file from the start directory and not from the root.
std::string fromLaunchDir(const std::string& path);
} // namespace Paths
#endif
