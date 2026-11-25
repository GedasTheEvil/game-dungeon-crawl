# Portable Linux release: runs on older distros (Mint 22, glibc 2.31+)

Status: solved 2026-11-25. Was: draft 2026-11-25, size L, needs nothing first.

## Problem

The release zip (`make release`, `tools/release.sh`) fails on Linux Mint 22 (the kid's PC): `libsdl3-0` is missing.
Bundling SDL3 alone would not fix it. Facts (2026-11-25, `build/release/bin/game` 1.6.1):

* The binary is built on Ubuntu 26.04 (glibc 2.43, GCC 15) and needs **GLIBC_2.43** (`sqrtf`, `atan2f`,
  `remainderf`; GCC 15 + new glibc headers pick the new symbol versions), GLIBC_2.38 (`__isoc23_strtol`, `fmod`) and
  GLIBCXX_3.4.31. Mint 22 = Ubuntu 24.04 base: glibc 2.39, libstdc++ from GCC 14. The loader refuses the binary before
  it looks for SDL3 ([same failure elsewhere](https://github.com/migriv24/VoidHormiga/issues/4)).
* Ubuntu 24.04 has no SDL3 package (it starts with 25.04). Ubuntu's `libsdl3-0` also links many libs directly
  (`ldd` lists ~50: X11, Wayland, PulseAudio, libsndfile, FLAC, ...), so copying the system `.so` drags a chain along.
* Rule: a binary built against an old glibc runs on newer ones, never the other way round. Build on the oldest base
  we support; ship what that base lacks.

## Options looked at

| Option | Verdict |
|---|---|
| Ship `libSDL3.so` from this PC | No: glibc 2.43 binary and lib, plus its ~50 direct deps. |
| Ask the target to install SDL3 (PPA, build from source) | No: per machine, needs root, still fails on glibc. |
| **Build in an old-glibc container, build SDL3 from source, ship it in `lib/`** | **Chosen.** What SDL and current SDL3 games do. |
| Fully static binary (`-static`) | No: static glibc breaks `dlopen` of GL drivers and SDL's backends. |
| Flatpak (Freedesktop 25.08 runtime has SDL3; Mint has Flathub) | Later, maybe. Manifest, sandbox, saves move to `~/.var/app/...` (the game writes `saves/` next to itself), the target needs the runtime download (~hundreds of MB). |
| AppImage | Optional wrapper on top of the chosen build (one file to double-click). Same build problem underneath; adds FUSE/runtime quirks. Not needed: zip + `Play` works. |
| `zig c++ -target x86_64-linux-gnu.2.31` | Fallback only: libc++ instead of libstdc++, SDL3 and FreeType built with zig too. |

Sources: [SDL3 README-linux](https://wiki.libsdl.org/SDL3/README-linux) ("by default SDL will only link against
glibc", the rest is loaded at run time; Steam Runtime SDK recipe), [Steam Runtime sniper release
notes](https://gitlab.steamos.cloud/steamrt/steamrt/-/wikis/Sniper-release-notes) (gcc-14 preinstalled, toolchain
file `/usr/share/steamrt/cmake/gcc-14.cmake`), SDL3 game ports that ship this way
([SpaceAnimalsRecomp](https://github.com/SerenityG4K/SpaceAnimalsRecomp/pull/39): sniper SDK as the build root, only
SDL3 shipped, RUNPATH `$ORIGIN/lib`; [God-Hand-Rearmed](https://github.com/God-Hand-Rearmed/God-Hand-Rearmed/pull/3):
CI runs the build on Debian 11, Ubuntu 20.04/24.04, Fedora, Arch), [Freedesktop SDK
25.08](https://discourse.flathub.org/t/freedesktop-sdk-25-08-0-released/10399) (SDL3 in the runtime).

## Plan

Build base: the **Steam Runtime 3 "sniper" SDK** image (`registry.gitlab.steamos.cloud/steamrt/sniper/sdk`, pin a
dated tag or digest): Debian 11 based, glibc 2.31, GCC 14.2 preinstalled, made for exactly this. Docker is on the dev
PC (the user is in the `docker` group). Target: any x86-64 distro with glibc >= 2.31 (Ubuntu 20.04+, Mint 20+,
Debian 11+, Fedora 33+, SteamOS).

1. **Compiler check first** (decides the rest). The game is C++23 and built with GCC 15 today. Build it with `g++-14`
   in the sniper image. Only `<ranges>` is used of the newer library parts (no `std::print`, `<expected>`, ...), so it
   should pass; fix the few GCC 15-only spots in the code if any. If GCC 14 cannot build it, fallback: a derived image
   with a newer GCC (or Ubuntu 22.04 + toolchain PPA, glibc 2.35), and say so in the plan.
2. **SDL3 from source** in the container: the same 3.4.x as the dev PC (`libsdl3-0 3.4.2`), tarball pinned by
   version and sha256 (downloaded into `build/release-deps/`, not committed). CMake defaults keep X11, Wayland,
   PulseAudio, PipeWire, ALSA as run-time `dlopen`: the built `libSDL3.so.0` should need only glibc (check with
   `objdump -p`). Shared, not static: zlib-licence either way, shared keeps the `.so` swappable.
3. **FreeType**: link it statically (built from a pinned tarball, without harfbuzz, brotli, bzip2, png: the game
   bakes plain TTF glyphs), or ship `libfreetype.so.6` from the container. Prefer static: one lib less to load.
4. **Link the game**: `-static-libstdc++ -static-libgcc` (the target's libstdc++ is then irrelevant), RUNPATH
   `$ORIGIN/../lib` (`-Wl,-rpath,'$$ORIGIN/../lib'` in the makefile), keep `-march=x86-64-v2`. Drop `-lGL`: GL is
   loaded through `SDL_GL_GetProcAddress` (`src/game/gl_window.cpp`), SDL `dlopen`s `libGL.so.1`/EGL itself; check
   nothing else needs the link (the tools keep theirs).
5. **Make and release.sh**: a `make release` path that runs the build inside the container (`docker run --rm -v
   $PWD:/src -u $(id -u):$(id -g)`, build dir `build/release` as now). Keep a `RELEASE_HOST=1` (or similar) escape to
   build on the host as today. `release.sh` copies `lib/libSDL3.so.0` into the zip and **fails** when
   `objdump -T bin/game lib/*.so*` shows a `GLIBC_` version above 2.31 or any `GLIBCXX_`/`CXXABI_` (the static
   libstdc++ must not leak), and when `ldd`-style `NEEDED` lists anything outside the allow list (libc, libm,
   libmvec, libdl, libpthread, ld-linux, libSDL3).
6. **Install-check**: after this, `bin/game` needs only glibc and a GL 3.3 driver (Mesa or vendor). Update the
   check: glibc version (`ldd --version` or `getconf GNU_LIBC_VERSION`) against the release's floor, the bundled
   `lib/` present, a GL driver hint (`libGL.so.1`/`libEGL.so.1` found by `ldconfig -p`). The soname -> apt table
   mostly shrinks; keep the Mesa/GL rows and the audio hint (no sound server: the game runs silent). Update
   `tools/install_check_test.sh`.
7. **Prove it on the targets** (containers, no VM needed): a `tools/release_smoke.sh` that unzips the release in
   `linuxmintd/mint22-amd64` (or `ubuntu:24.04`), `ubuntu:22.04`, `debian:11`, `fedora:latest`, installs only
   `xvfb` + Mesa (`libgl1-mesa-dri`), and runs `xvfb-run -a timeout -k 5 20 bin/game --no-sound` with a scenario or
   `--level 1` that quits by itself; pass = exit 0 and no `not found` in `ldd`. Sound off, as AGENTS.md wants for
   agent runs. Not part of `make check` (pulls images); run by `make release` only when asked (`ARGS=--smoke`).
8. **Docs**: development.md (release line: built in the sniper container, glibc floor, `lib/`, smoke script),
   where-is-x rows (container build, smoke script), glossary only if a new term appears ("glibc floor"?).
   Readme/manual: "runs on Linux x86-64 with glibc 2.31+ and OpenGL 3.3".

Version: tooling only (`.version` unchanged), unless the code needs GCC 14 fixes (then still no gameplay change).

## Open points (implementer's choice)

* Sniper SDK vs plain `debian:11` / `ubuntu:22.04` + toolchain: sniper is preferred (GCC 14 ready, Valve keeps it
  patched); a plain image is the fallback if the registry is slow or GCC 14 fails.
* FreeType static vs shipped `.so` (point 3).
* AppImage wrapper on top: separate idea, not in this plan.
* Windows build (mingw): out of scope; save as its own draft if wanted.

## Check on the target

* Mint 22 (the kid's PC): unzip, `./Install-check` says all good, `./Play` starts with sound and music.

## Result (2026-11-25)

Done as planned, with these differences and findings:

* **Compiler:** GCC 14.2 in sniper builds the game as is (no code change for it). Digest-pinned image
  (`tools/release_build.sh`, `SNIPER_IMAGE`).
* **SDL 3.4.20, not 3.4.2.** SDL 3.4.2 found no GLX visual on Mesa 25.x under Xvfb (Mint 22, Fedora: "Couldn't find
  matching GLX visual" for every attribute set; EGL worked; Fedora's own SDL 3.4.20 worked). 3.4.20 (same ABI) works
  everywhere. Both tarballs gpg-checked (Sam Lantinga, Werner Lemberg) when pinned.
* **Not the image's toolchain file:** with `/usr/share/steamrt/cmake/gcc-14.cmake` SDL's Wayland and D-Bus checks see
  no pkg-config and drop both silently. Built with `CC=gcc-14 CXX=g++-14`; `release_build.sh` fails when X11,
  Wayland, libdecor, KMSDRM, D-Bus, PulseAudio, PipeWire or ALSA is off.
* **FreeType 2.14.1 static** (no zlib, png, bzip2, brotli, harfbuzz).
* **No libGL link:** the render library asked `glXGetCurrentContext` whether GL objects may free themselves (wrong on
  EGL / Wayland too). Now `glContextCurrent()` is a flag in `render/gl/gl_context.*` (`initGlContext` sets it,
  `endGlContext` in `GlWindow::close` clears it).
* Link: `-pthread` needed on glibc 2.31; SDL's pkg-config adds an RPATH to the build tree, so `SDL_LIBS` is given by
  hand. Result: `bin/game` and `lib/libSDL3.so.0` need GLIBC_2.29 at most, no GLIBCXX, NEEDED only glibc and SDL3.
* Build dirs: container `build/release` (dropped when the compiler changes), `--host` `build/release-host`.
* **Smoke:** Debian 11 replaced by Ubuntu 20.04 (also glibc 2.31; Debian 11's security archive no longer serves).
  `make release ARGS=--smoke`: Mint 22, Ubuntu 22.04, Ubuntu 20.04, Fedora 45 pass (Install-check all good, the
  scenario PASS, screenshot renders on llvmpipe).
* Install-check: glibc floor (`release.sh` writes the release's into the zip's copy), bundled `lib/` present, a
  GL/EGL driver in `ldconfig -p` (else `libgl1 libgl1-mesa-dri`); the soname table shrank to SDL3, FreeType, GL,
  libstdc++, libgcc, libc (the `--host` build's needs).
* Not testable here: sound on a real PC (containers run `--no-sound`). The "Check on the target" below stays for the
  user.
