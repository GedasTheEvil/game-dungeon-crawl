# Sandstorm halls

Status: implemented 2026-11-02, waiting for the user's play-test (`bin/game --play tests/play/sandstorm-halls.txt`). Was:
draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Opus: a level feature, visuals and level design. [desert-chapter](solved/desert-chapter.md) left the sandstorm halls out;
only a sandstorm monster or level flag exists (`monster_kinds`, `level.h`): check what it does today.

* A hall in the desert zone where wind blows sand: pushes the player and light monsters sideways in gusts, lowers
  view distance (haze), drowns sound; arrows drift. Gusts on a rhythm with a tell (rising hiss, grit), so the player
  can time crossings, like the rhythm traps.
* Shelter: pillars and alcoves block the wind.
* Place 2-3 in the desert levels; checker models the push (or treats the hall as passable with a warning-free rule).
* Sounds via `tools/audio/sand_sounds.py`.

## Implemented

What existed: no sandstorm level flag, only Seth's storm roar and Rerek's spray (`Dungeon::sandBurst`, the sand haze over
the screen).

* **The sand gust** (`SandGust`, tile type 24, a wall-layer fixture, legend `{`): the attribute is the speed (0 normal,
  1 fast, 2 slow) plus `GUST_LEFT` (3) for a wind to the left; the value the phase, the live state above it, as the
  rhythm traps. Its own cycle in `spike_cycle.h` (`GUST_PERIOD_MS` 7 / 5.6 / 8.4 s, `GUST_OUT_MS` 2 / 1.6 / 2.4 s,
  `GUST_TELEGRAPH_MS` 1.2 s), ticked with the rhythm traps (`isCycled`); not a rhythm trap (it does not hurt).
* **The push** (`world/dungeon_gusts.cpp`, `Dungeon::updateGusts`): in a blowing gust's cell the player goes downwind
  at `GUST_PUSH_SPEED` 1.6 tiles a second, faster than the walk (1, in sand 0.75), so against the wind they lose
  ground; not on a ladder, not held; walls stop it. The first push says so and writes the field note Sandstorm. The
  light monsters (weight up to 1: rats, scarabs, scorpions, camel spiders, the bats; walkers, leapers and flyers awake,
  no boss) go at 1.2; a walker stops where it would not walk.
* **Shelters**: the cells of a hall without a gust. The game draws a sandstone pillar (a pilaster with a base and a
  capital, the wall's texture) against the back wall on the side the wind comes from (`Dungeon::windbreakSide`,
  `drawWindbreak`), up through a tall hall. No Blender model: level geometry like the walls. No alcove geometry: the
  shelter is the pillar's lee.
* **Haze and sound**: `Dungeon::StormHaze` follows the gusts within 4 tiles on the player's row (thin in the calm,
  rising with the tell, thick in a gust; `GUST_HAZE_*`); the zone's fog turns to blown sand and closes in past the
  player (`STORM_FOG*`, `graphics/draw.cpp`). In a gust `SoundMuffle` takes up to 65% of the other sounds' volume
  (`Sound::Play(gain)`, which now sets its channel's volume each play), not the wind's own. The emissive shader fogs a
  premultiplied sprite by its alpha (its clear corners showed as squares in thick fog).
* **Arrows drift**: the player's missiles (`Missile::drift`) and the monsters' shots go `GUST_DRIFT_SPEED` 2.5 tiles a
  second downwind while in a blowing gust.
* **The look**: `Grit::gust` per gust cell (`Dungeon::drawGusts`): a few motes in the calm, grit hopping low along the
  floor through the rise, streaks of sand at every height and rolling dust in the gust.
* **Sounds** (`tools/audio/sand_sounds.py`): `gust_rise.wav` (the tell, 1.2 s: a swelling hiss, grit) and
  `gust_blow.wav` (2.4 s: a rush, a howl, sand rattling); the older three files come out the same.
* **Checker** (`checkSandGusts`, `gustsCrossable`): a gust is a cost on the path (`COST_GUST`, 0.3 difficulty each);
  warnings: a run that cannot be crossed against the wind in one calm (`GUST_CROSS_MARGIN_MS` to spare, at the sand
  walk if the run has sand: in sand 2 cells at normal speed, on stone 3), blows both ways, has no floor under it, spikes
  or a death trap or quicksand in it, or blows the player into a drop, a trap or quicksand at its downwind end.
  `levelcheck --script` waits for the calm before a run entered against the wind (`waittraps`, which walks a gust run
  at the sand speed).
* **Levels**: three halls, all on the exit path, the wind against the walk, upwind runs a phase ahead (the gust rolls
  down the hall): lvl33 row 40 (sand, the wind to the right, runs of 2, camel spiders round it), lvl34 row 31 (stone,
  to the left, runs of 3, giant scorpions), lvl36 row 15 (stone, slow, to the left, runs of 3). All levels check clean;
  the decor goldens of 34 and 36 changed on those rows (no scattered prop in a gust's cell).
* **Editor**: the hint (wind and phase lists), the readme row; the map's sandstorm mark (three lines of wind, an
  arrowhead); journal note `note.sandstorm`.
* **Tests**: `tests/unit/sandstorm_test.cpp` (`tests/levels/sandstorm`), scenario `tests/scenarios/sandstorm.txt`
  (lvl34 through a gust cycle, lvl33's sand hall).
* Not done: a unit test of the arrows' drift and of a light monster blown (seen in play only).

