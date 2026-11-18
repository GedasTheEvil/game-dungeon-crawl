# Audio on SDL3: drop SDL 1.2 and SDL_mixer

Status: implemented 2026-11-18, waiting for the user's play-test. Was: draft 2026-11-18, size M, needs nothing first.

## Problem

The game's audio (`src/audio/sound.cpp`, `src/game/game.cpp`) runs on SDL 1.2 and SDL_mixer 1.2, unmaintained since
about 2012. It is also the blocker for the window move: SDL 1.2 and SDL3 cannot be linked into one binary (same
symbol names), so audio moves first, while the window is still GLUT
([sdl3-window-input](sdl3-window-input.feature.draft.opus.md) comes next).

Ubuntu 26.04 packages SDL3 (`libsdl3-dev` 3.4) but not SDL3_mixer. The game uses little of SDL_mixer: `LoadWAV`,
`LoadMUS` (14 `.ogg` tracks), play and fade-in on a channel, halt, volume per channel, reserved channels, music
fade in and out, music volume, `PlayingMusic`.

## Plan

* `libsdl3-dev` (3.4.2) is installed.
* No SDL3_mixer: a small mixer of our own on SDL3 audio streams, in `src/audio/` (the only module that touches it):
  * one `SDL_AudioStream` per channel, all bound to one opened playback device (`SDL_OpenAudioDevice`,
    `SDL_BindAudioStreams`); SDL3 mixes bound streams and converts formats itself;
  * per channel: gain (`SDL_SetAudioStreamGain`) for volume, a fade as a gain ramp stepped from the game's update
    tick, halt = `SDL_ClearAudioStream`; reserved channels kept as now;
  * sound effects: `SDL_LoadWAV`, converted once to the device format at load;
  * music: `.ogg` decoded with `stb_vorbis` (vendored in `external/stb/`, like `stb_image`), streamed into its own
    stream from a callback (`SDL_SetAudioStreamGetCallback`), looping; fade in/out as for channels.
* Keep the `Sound` API (`src/audio/sound.h`) as is, so no caller changes; only its implementation moves.
* `--no-sound`: the dummy driver (`SDL_AUDIO_DRIVER=dummy` in SDL3, was `SDL_AUDIODRIVER`); scenarios and unit
  tests stay silent.
* `SDL_Init(SDL_INIT_AUDIO)` only; GLUT keeps the window until the next plan.
* makefile: `SDL_LIBS` from `pkg-config sdl3`; docs: [development.md](../development.md) dependencies and the release
  zip note (`libsdl3-0` instead of SDL 1.2 and SDL_mixer).
* Unit test: the mixer's gain ramp and channel bookkeeping on the dummy driver.

## Play-test

* `bin/game --level 1`: music starts with its fade-in, footsteps, doors, monsters and pickups sound as before.
* Change music and effect volume in Options: both follow at once.
* Go to the next level: the music fades out and the next track fades in, with no click or gap.

## Done

* `src/audio/mixer.*`: `Mixer` (8 channels, channel 0 reserved for the ambience, one music stream; a looped clip is
  put again from the stream's get-callback, the music decoded there) and `GainRamp` (the fades, stepped by
  `Mixer::Update`, which `Music::Play` calls each update). `src/audio/sound.*` keeps its API on top of it.
* `external/stb/stb_vorbis.c` v1.24 (github.com/nothings/stb master, 2026-11-18), compiled in `external/stb/stb.cpp`.
* Unit test `tests/unit/mixer_test.cpp` (dummy driver). Checked once on SDL's disk driver: the mixed music matches
  an ffmpeg decode of `menu.ogg` sample for sample at gain 1 (correlation 1.0), the fades and the effects show in
  the levels.
