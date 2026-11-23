# Installation helper script in the release (checks libs, suggests apt install)

Status: solved 2026-11-23 (play-tested by the user). Was: implemented 2026-11-21, waiting for the user's play-test. Was: draft 2026-11-20, size S, needs nothing first.

## Idea

The release zip (`make release`, `tools/release.sh`) is carried to another PC (the kids' one) that may lack the
shared libs `bin/game` needs. A script in the release checks what the system has and prints one `apt install <list>`
command for the missing packages. It never installs anything itself (the user runs the printed command).

Targets for now: Ubuntu and Linux Mint (apt, amd64). Other distros: print "unsupported, libs needed: ..." only.

## Plan

* `Install-check` (root of the release, next to `Play`; plain `sh`, no dev tools, same style as `Play`), shipped by `tools/release.sh`.
* Find the libs: run `ldd bin/game` (and `bin/editor`, `bin/model-viewer` if shipped), collect the `not found` sonames.
* Map soname -> apt package. Prefer a static table in the script (libSDL3, GL/Mesa, audio, ...) built from the real
  `ldd` of the release binary; fallback for unknown sonames: `apt-file`-free hint, list the soname.
* Check packages with `dpkg -s` (so a package missing but its lib present by other means still works: the `ldd` result decides).
* Also check runtime needs `ldd` can't see: a GL 3.3 core driver (`glxinfo` optional, only a hint), audio device.
* Detect the distro from `/etc/os-release` (`ID`, `ID_LIKE` contains `ubuntu`/`debian`). Mint is `ID=linuxmint`, `ID_LIKE=ubuntu`.
* Output: "All good, run ./Play" or `sudo apt install a b c`. Exit code 0 / 1 so `Play` could call it on a launch failure (optional).
* Test: unit-style shell test with a fake `ldd` output (PATH stub) for missing / nothing missing / non-apt distro. Check on a clean Ubuntu container if available.
* Docs: development.md release line, where-is-x row, glossary if a term is added. Tooling-like: no version bump if only the script; the release content changes, so decide at implementation.

## Play-test

* Unzip the release on a PC without the dev libs (or a fresh Ubuntu / Mint VM): run the script, run the printed `apt install`, run it again (all good), then `./Play` starts (stand-in play command on a dev PC: `bin/game --level 1`, the check script is `./Install-check`).
