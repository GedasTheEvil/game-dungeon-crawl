# The desert chapter: Seth's sands

Status: draft 2026-10-19, from the user's review of the campaign ("I love the desert chapters"). Needs first:
[hippo-boss](hippo-boss.draft.opus.md) (Seth, the chapter's boss). Works better with
[sunlight-mirrors](sunlight-mirrors.draft.opus.md) (the chapter's lock) and [placed-decor](placed-decor.draft.opus.md)
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

Full 40 x 47 grids from the start ([denser-levels](denser-levels.draft.opus.md)). Sandstone, not the tomb's
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
* **Lock: the sun.** The chapter's gates open with [sunlight-mirrors](sunlight-mirrors.draft.opus.md) (a shaft of sun
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
both sides, palms and reeds as decor. Seth as in [hippo-boss](hippo-boss.draft.opus.md); the chapter adds his storm
in the land: the storm roar throws sand (if the storm roar is the chosen ability). Behind the boss gate the key to
the exit, as the other boss levels.

## Journal

* A field note on sand ("Sand. Underground. Somebody left a door open, three thousand years ago.") and on quicksand
  the first time the player sinks.
* [duat-hours](duat-hours.draft.sonet.md) (on hold): its 12 hours spread over 35 levels; the desert can be the hour
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
