# Release zip for testing

Status: draft 2026-11-06, size S, needs nothing first.

## Problem

To test the game on a system without dev tools, the user copies the compiled game there by hand. The root `Play`
script starts it from a folder that has `bin/`, `media/`, `content/` and `saves/` ([docs-root-tidy](solved/docs-root-tidy.tooling.md)).
There is no script that makes that folder.

## Plan

* `tools/release.sh` (bash, `set -eu`), plus a `make release` target that runs it. Steps:
  1. Version: the same string the leaderboard uses, `git describe --always --dirty` (`GAME_VERSION` in the makefile).
     Refuse a dirty tree (exit 1, with a message) unless `--allow-dirty` is given, so that a zip always matches a
     commit.
  2. Build (see "Portable build" below).
  3. Create `release/` in the repo root. Stop if it already exists (it is left over from a failed run: report it,
     don't delete it silently).
  4. Copy `bin/game` (only the game: no tool binaries), `media/`, `content/` and the `Play` script into `release/`.
     Leave out `.directory` files.
  5. Create an empty `release/saves/`.
  6. Zip the contents of `release/` (not the folder itself, so `Play` sits at the top of the zip) into
     `release-<version>.zip` in the repo root. Check that the empty `saves/` is in the zip (`unzip -l`). `zip -r`
     stores directory entries; if it doesn't, add `saves/` explicitly.
  7. Remove `release/` (also on failure after step 3: `trap`).
  8. Print the zip's path and size.
* `.gitignore`: `/release-*.zip` and `/release/`.
* Portable build: the makefile compiles with `-O3 -march=native`, so a binary built here can crash with "illegal
  instruction" on an older CPU. Build the release with `-march=x86-64-v2` (or no `-march`) into its own build and
  bin folders, so the dev build's objects are not rebuilt or mixed up. For example a `RELEASE=1` makefile switch that
  sets the flags, `BUILD=build/release`, `BIN=build/release/bin`. The release build makes only the game target, and the script
  copies that `game` binary as `bin/game`. Without a separate build, `make release` would force a full rebuild of the dev tree twice.
* Docs: development.md (Build and run: `make release`, what the zip holds, that the target needs SDL, GL and
  GLUT installed). Glossary row "release zip".
* Test: run `make release` and unzip into a temp folder. Then run
  `xvfb-run -a timeout -k 5 10 ./Play --no-sound` from the unzipped folder: it must start and write `game.log` there.
  `make check` passes.

## Decisions and open points

* No tool binaries (user, 2026-11-06): `bin/` in the zip holds `game` only, and `LevelEditor` and `ViewModel` stay out.
* `media/sounds/music/soundtrack.ogg` (the music fallback) is untracked and local. Copy it if it is there. The zone
  tracks are tracked and always ship.
