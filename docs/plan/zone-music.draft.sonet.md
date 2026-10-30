# Zone music

Status: draft 2026-10-30, needs nothing first. The code works with no new files: every zone and the menu fall back
to `soundtrack.ogg`. The music itself comes later, from the user (sources below).

Sonnet: one lookup table and a music switch, the pattern of the zone ambience (plan zone-looks).

## Goal

One music file per zone and one for the main menu. `media/sounds/music/soundtrack.ogg` stays the fallback for any
zone or the menu without a file of its own.

Today: `Assets::sounds.soundtrack` (`src/state/assets.cpp`) loads `soundtrack.ogg` once; `GameState` starts it
after the cache load (`src/state/game_state.cpp`) and it loops for the whole session. `Music` (`src/core/sound.h`)
can only load once and play.

## Files

* `media/sounds/music/menu.ogg`: the main menu (also the death and win screens, unless the implementer finds a
  better fit).
* `media/sounds/music/<zone slug>.ogg`: the zone's levels, slug from `ZONE_INFO` (`src/world/campaign.h`):
  `tomb_entrance.ogg`, `bat_galleries.ogg`, ..., `hall_of_judgment.ogg`.
* A level with no zone (test levels, `gen:SEED:D`) plays the fallback.
* Missing file: fallback, one log line at load (not every frame).
* Several zones may share one track: a later copy or symlink is enough, no table for it.

## Code

* `Music`: load by file name on demand, keep the name playing; `Play(filename)` like `Ambience::Play`: same file
  goes on, another fades out and the new one fades in (`Mix_FadeOutMusic` / `Mix_FadeInMusic`, ~1.5 s), empty or
  missing falls back to `soundtrack.ogg`. Resolve the fallback once per file name (cache the existence check), not
  per call.
* Pick the track where the ambience is picked (`src/state/game_loop.cpp`, `dungeon.Look().ambience`): from the
  level's zone (`dungeon.meta.zone`). The main menu picks `menu.ogg` while it is open.
* Changing level inside one zone keeps the track running; crossing into a new zone fades.
* Volume stays `settings.sound.music`; `--no-sound` stays silent.
* Unit test: the file choice (zone, no zone, menu, missing file → fallback) as a pure function, no audio device.
* docs/development.md (`media/sounds/` line): the new names and the fallback.

## Licensing (research 2026-10-30)

`soundtrack.ogg` is non-free and not in git (`.gitignore`). New tracks should be committable: the repo is on
GitHub, so a committed file is a file anyone can download on its own. That rules out licences that forbid
"standalone" redistribution.

Usable, safe to commit:

* **CC0** (no credit needed). OpenGameArt filtered by CC0: "Egyptian meditation music" (CC0 / OGA-BY, temple,
  mellow), "Desert_mystic (dynastic Egyptian theme)" in the RPG::Music collection, "Forgoten tomb ambience",
  "Ancient Power Of Serpents", "Desert theme" (CC0 Music collection), "Dungeon Ambience". Free Music Archive:
  HoliznaCC0 (game soundtrack albums). itch.io tag CC0 + music (e.g. Tallbeard Studios loop bundle). Freesound
  music category, CC0 filter (mostly loops and samples). Check the licence on each track's own page, not the
  collection's.
* **CC-BY 4.0 / OGA-BY** (credit needed). Kevin MacLeod, incompetech.com (large catalogue, some "world" and
  dark ambient tracks): credit line names track, author, site, licence. Credits screen is a PNG today
  (`media/textures/ui/credits.png`): the music credits need a text list (`media/sounds/music/CREDITS.md` in
  git and a line on the credits sheet or screen).
* **CC-BY-SA**: fine for files, but adaptations (loops, cuts) must stay CC-BY-SA.

Avoid:

* **NC / ND** licences (much of Free Music Archive, Tabletop Audio: CC BY-NC-ND): no commercial use, no edits.
* **Pixabay Content License**: free in a game, but no standalone distribution; a raw `.ogg` in a public repo is
  close to that. Only Pixabay uploads from before 2019-01-09 are CC0 (their CC0 filter).
* **AI generators** (Suno etc.): paid plans grant commercial-use rights but, since the 2025-12 Warner deal, not
  ownership, and no copyright vests in pure AI output. Usable, but the licence is Suno's terms, not a free licence;
  treat like Pixabay (keep out of git, like `soundtrack.ogg`) unless their terms clearly allow redistribution.

Other options: commission a composer (CC-BY or full rights, e.g. via OpenGameArt or itch.io), or compose in-house
(the effects are synthesized in `tools/audio/`, but a full score that way is a big, separate task).

Each committed track: note source URL, author, licence in `media/sounds/music/CREDITS.md`.
