# The desert chapter: Seth's sands

Status: draft 2026-10-19, from the user's review of the campaign ("I love the desert chapters"). Needed first
[hippo-boss](hippo-boss.md) (Seth, the chapter's boss), now in. Implemented 2026-10-20, see
[Implemented](#implemented); play-test: `bin/game --level 21` (to 25). Solved 2026-10-20, after sandier ceilings. Works better with
[sunlight-mirrors](../sunlight-mirrors.draft.opus.md) (the chapter's lock) and [placed-decor](../placed-decor.draft.opus.md)
(sand drifts placed by hand); does not need them.

## Why

Levels 16-25 are one long wet stretch: flooded halls, crocodiles, cobras, two water bosses. The campaign has desert
creatures (camel spider, sand worm, scorpions) but no desert. Seth, god of the desert and the storm, has no slot.

## Placement

Five new levels between Apep (lvl20) and the flooded halls (today lvl21): the desert breaks the water stretch in two.
The campaign grows from 30 to 35 levels (`CAMPAIGN_LEVELS`, `src/world/campaign.h`); today's lvl21-30 become
lvl26-35, a boss every five levels as today:

| Levels | Chapter | Boss |
|---|---|---|
| 1-20 | as today | Khepri, Bat of Shezmu, scorpion queen, Apep |
| 21-25 | the desert (new) | Seth (lvl25) |
| 26-30 | the flooded halls, today 21-25 | Sobek (lvl30) |
| 31-35 | the necropolis, today 26-30 | mummy priest (lvl34, as today in 29), Anubis (lvl35) |

Renumbering moves level files, the `docs/levels.md` table, and every test, play script and level doc that names a
level from 21 on (grep `lvl2[1-9]`, `lvl30`, `--level 2`, `--level 30`). The ankh and the finale stay last.

Story: Apep's pit opens up into a buried temple of Seth, sand blown in through the cracks of a sky the player never
sees. At the bottom, Seth's sacred pool (an oasis) where he lurks as the red hippo; the water from his pool feeds the
cisterns that follow.

## The chapter

Full 40 x 47 grids from the start ([denser-levels](../denser-levels.draft.opus.md)). Sandstone, not the tomb's
limestone: own wall and floor textures, warm light, light shafts from cracks above.

* **Sand** (new structure, beside `HalfWater`): drifted sand on the floor. The player walks through it at 3/4 speed
  and cannot sprint in it (jumping is fine). Shown as a raised, rippled floor, dust puffs on each step.
* **Quicksand** (a floor trap in sand): looks like sand, a tell up close (a slow swirl). Standing on it sinks the
  player: slowed more each second, after ~3 s stuck, after ~5 s damage per second until they jump out. Jumping out
  costs stamina. `levelcheck` treats it as a passable trap.
* **Sand falls** (a rock-fall variant): sand pours from a ceiling crack and buries the cell under a drift; blunt
  damage and a short slow, no stones. Same trigger as the rock fall.
* **Sandstorm halls** (implementer's choice, one or two levels): wind along the row pushes the player and blurs the
  far view; it blows in gusts, a rhythm-trap clock (`period`, `phase`), so the player walks between gusts.
* **Lock: the sun.** The chapter's gates open with [sunlight-mirrors](../sunlight-mirrors.draft.opus.md) (a shaft of sun
  through a crack, bronze mirrors) rather than colour keys: one or two colour locks per level at most. Without the
  mirrors plan in: levers, and the mirrors come later.

## Monsters

* Today's desert set: camel spiders, giant scorpions and scorpions, sand worms, giant cobras, giant bats. Few
  mummies, no crocodiles (no water until the oasis).
* **Sand worm: burrows in sand.** In a sand cell it lurks under the surface (the hippo's submerged lurk, a moving
  ripple), comes up under the player. Out of sand it is as today. Boss rule: no boss has the worm as kin; if one is
  added later, it burrows too.
* New monster, implementer's choice of one (or none, if the chapter plays well without):
  * **Horned viper**: a small desert snake that hides in sand (only its horns show), fast lunge, poison. Kin of the
    cobra family; Apep, the cobra boss, gains nothing (a lunge he has).
  * **Vulture of Nekhbet**: a flyer for the high sand halls, swoops along the row and climbs away. If it gets a
    boss later, the boss swoops farther.

## Seth (lvl25)

The boss level ends in Seth's oasis: a sealed chamber with a pool (deep water, half water round it), sand ledges on
both sides, palms and reeds as decor. Seth as in [hippo-boss](hippo-boss.md); the chapter adds his storm
in the land: the storm roar throws sand (if the storm roar is the chosen ability). Behind the boss gate the key to
the exit, as the other boss levels.

## Journal

* A field note on sand ("Sand. Underground. Somebody left a door open, three thousand years ago.") and on quicksand
  the first time the player sinks.
* [duat-hours](../duat-hours.draft.sonet.md) (on hold): its 12 hours spread over 35 levels; the desert can be the hour
  of the Gate of Sand.

## Tests

* Unit / sim: sand slows and blocks sprint, quicksand sinks and hurts and a jump frees, the sand fall buries and slows,
  the worm lurks only in sand, the campaign has 35 levels and the boss slots above.
* `bin/levelcheck content/levels/lvl*` clean, all 35 levels (new checks: quicksand, sand, worms only lurk in sand).
* Scenario with screenshots: a sand hall, quicksand's tell, the oasis.

## Open (warn when reporting)

* 35 levels, the desert at 21-25 (default), or the desert at 16-20 after the scorpion queen (scorpions lead into
  the sand; the water stretch then stays one block).
* Which new monster, if any (horned viper, vulture).
* Sandstorm halls: in or not.

## Implemented

Choices for the open points: 35 levels, the desert at 21-25; no new monster; no sandstorm halls; levers and colour
keys (two lock colours a level), no mirrors.

* **Campaign**: `CAMPAIGN_LEVELS` 35. The old lvl21-30 are lvl26-35 (level files, their drawings in
  `tools/level/campaign/`, the decor goldens, the inscription headers, the play scripts and docs that named them).
  Bosses: Seth lvl25, Sobek lvl30, the mummy priest lvl34, the Anubis boss lvl35 (the ankh). Renamed files get a new
  decor seed (the file name): their props, torches and decals are scattered anew (goldens rewritten). A seventh level
  gem, electrum, for 31-35. Five more royal names (`content/names/kings.txt`, 35 now: one per level).
* **Sand** (`Structure::Sand`, `,`): the walk at `SAND_SPEED_FACTOR` 0.75, no sprint (`Dungeon::PlayerCanSprint`), a
  jump is fine; a step kicks up dust and sounds (`SandStep`). Drawn as a rippled drift over the floor
  (`drawSandDrift`): it runs out towards dry floor, piles against rock and the back wall.
* **Quicksand** (`Structure::Quicksand`, `%`): as sand to look at, darker, and up close (2.5 tiles) its sand twists
  slowly round the middle (the tell). Standing in it: the walk from 0.5 down to nothing at 3 s, 5% of the max HP a
  second from 5 s (no armour), the player drawn sinking (`QuicksandSink`, to 0.2 tiles). A jump frees them for 40
  stamina (twice a jump's). `levelcheck`: a passable trap, jumped like spikes, at most 2 cells in a row.
* **Sand falls**: a `RockFall` with attribute 1 (`SAND_FALL`, `&`): the crack, the trickle and its hiss, then sand
  pours (`Grit::pour`) instead of a rock: 40 (no armour) and a 1.5 s slow to 40% for the player under it, the cell
  turns to sand.
* **The worm burrows in sand** (`MonsterKind::sandBurrow`): spawned in sand it lies under it (hidden, a heaving mound
  of sand where it is, `drawRipple`), comes up within 2.5 tiles under the player in its sand, or at its sand's edge for
  a player beside it; awake, it dives to come up under a player in its sand from 1.5 to 6 tiles away. Out of sand it
  walks as before. Boss rule: `validKinds` makes a boss of the worm's kin burrow too (none yet;
  [worm-boss](../worm-boss.draft.opus.md)).
* **Sandstone**: the desert's dressed rows are sandstone (`wall_sandstone`, `wall_sandstone_worn`, `floor_sandstone`,
  `tools/textures/surfaces.py`; `desertLevel`, `DESERT_FIRST` .. `DESERT_LAST`), no plaster. The ceilings (after the
  play-test, 2026-10-20: the first one looked like bricks): worn sandstone under a crust of sand, three variants that
  tile together, `ceiling_sandstone`, `_sandy` and `_cracked`. About one cell in six with rock above and below gets the
  cracked one and a shaft of daylight through it (`SunBeam::shaft`, a warm light). Sounds: `tools/audio/sand_sounds.py`.
* **The levels** (`tools/level/campaign/lvl21.txt` .. `lvl25.txt`, built with `ascii2level.py`): 13 halls each on
  the full grid, sand in every hall, quicksand singles and pairs, sand falls, worms in the sand, giant scorpions,
  giant cobras, camel spiders, giant bats, mummies, Anubis guards in 24 and 25, no crocodiles. Riddle gates in all
  five, dart traps in 22 and 23, name gates with sealed rooms in 22 and 24. Lvl25 ends in Seth's oasis: a pool (half
  water over deep water) between two sand ledges, lotuses, the Blessing of Hapi by the teleporter; behind the boss
  gate the gold key and the ladder up to the exit hall. Difficulty 182 to 191, between lvl20 (182) and lvl26 (192).
* **Journal**: field notes Sand and Quicksand; the worm's burrow move in its words; inscriptions 46-50 (Sinuhe in the
  desert, Seth in the prow, the ways of Rosetau, the red hippopotamus, the first harpoon with a might boon) and 51, 52
  for the sealed rooms.
* **Tests**: `tests/unit/sand_test.cpp` (sand, quicksand, the sand fall, the worm in sand, the 35-level campaign and
  the desert levels), scenarios `tests/scenarios/solved/sand.txt`, `sand_worm.txt`, `desert.txt` (screenshots; `desert.txt`
  uses the new `level N at COL ROW` to start in the oasis). `levelcheck --script` waits for a jump's stamina before each
  scripted jump.
* Not done: the sandstorm halls (left out), the sunlight mirrors (their own plan), a new desert monster. `make paths`
  fails on old levels as before (bats' Fatigue starves the scripted jumps); the desert paths play through on copies
  without monsters.
