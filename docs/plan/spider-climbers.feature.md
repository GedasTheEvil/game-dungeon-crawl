# Spider climbers, and a tiny spider

Status: implemented 2026-11-05, waiting for the user's play-test. Was: draft 2026-11-02, size L, needs nothing first. From the user.

## Idea (from the user)

Spiders should be climbers. The current spider (the camel spider) is a fast giant spider; add a "tiny" spider on the
same model.

## Today

* The camel spider (`MonsterCamelSpider`, glyph `h`, `src/world/monster_kinds.cpp`): fast (speed 28), weak (330 HP),
  a light shunner, scale 28. Its boss, the Mother of Webs, is on the same model (scale 70). Neither climbs.
* The model (`tools/blender/models/camel_spider.py`) has walk, idle and attack clips, no climb clip.
* Climbers: [monster-climbers](solved/monster-climbers.md), [climber-off-the-rungs](solved/climber-off-the-rungs.md),
  [climber-crosses-ladder](solved/climber-crosses-ladder.md) (`MonsterKind::climbs`, `climbRise`, `climbGrip`).

## Plan

### Camel spider climbs

* `.climbs = true` on the camel spider, with `climbRise` and `climbGrip` measured on the model like the other climbers.
* A climb clip in `camel_spider.py` (Blender 5, [modeling](../modeling.md)): eight legs on the rungs, body close to
  the ladder. The light rule still holds: a lit ladder top is a wall to it.
* Boss (AGENTS.md, Boss abilities): the Mother of Webs climbs too, faster (a quicker climb clip rate).

### Tiny spider

* A new `MonsterKind` on the camel spider's model and texture (or a darker tint of it): scale about 10, a free glyph
  (AGENTS.md, Glyphs). Name it in [glossary](../glossary.md).
* Weak and fast: a few hits of the starting weapon kill it, a small bite (suggest poison, weak), low threat and xp.
  Climbs and shuns the light like its big kin.
* Comes in groups (2 to 4 per spawn), early levels; sizes its `generated` range against the balance sim.
* When [subcell-monsters](solved/subcell-monsters.md) lands: the tiny spider fits through crawl spaces, the
  camel spider does not.
* Journal: a field note entry like the other monsters.

## Open points (implementer's choice, report them)

* Boss for the tiny spider: suggest none of its own; the Mother of Webs drops tiny spiders along with the camel
  spiders (`Summon::Drop`).
* Poison bite or plain bite for the tiny spider.
* Same texture scaled, or its own tint so the player tells it from a far camel spider.

## Tests

* Unit tests on the sim harness (`tests/unit/sim_world.h`): the camel spider and the tiny spider follow the player up
  a ladder; neither climbs to a lit ladder top.
* A play script `tests/play/spider-climbers.txt`: a ladder room with camel and tiny spiders.
* `bin/levelcheck content/levels/lvl*` stays clean (a climbing spider changes threat paths).

## Implemented

* The camel spider and the Mother of Webs climb (`.climbs`; the threat x1.05 comes with it). A climb clip
  `camel_spider_climb.md3` (`tools/blender/models/camel_spider.py`, 16 frames, back to the camera, head up, the
  tetrapods take turns, the palps on a rung over the head, 2 rungs a cycle; `climbRise` 0.783, `climbGrip` 0.1866,
  shared by the tiny spider and the Mother at their scales). The Mother climbs faster through the boss climb factor.
* Light: a light shunner's climb path (`ClimbAbility::avoids`, `Dungeon::followPath`) takes no lit cell, so a lit
  ladder top keeps it below. A climber left hanging over the shaft with no way on (it started up after the player
  on the rungs) climbs back down to the floor below (`Dungeon::climbDown`).
* Tiny spider: monster 36, glyph `[`, the camel spider's model with its own darker texture `camel_spider_tiny.png`
  (`--tiny-texture`), scale 10, speed 20, 18 HP (two club blows), a plain bite of 1 every 0.5 s, threat 0.6, 250 XP,
  light shunner, climber, `heightQuarters` 1 (crawl spaces). `MonsterKind::group` 4: levelgen puts 2 to 4 side by side
  (`placeGroup`, needs room for two). Zone fauna: the tomb entrance and the web halls; levelgen 1-6. Hand-placed in
  levels 2, 3, 4, 10, 11, 12 (dark floors). Field note `[monster.tiny-spider]`, glossary row.
* Open points chosen: no boss of its own; the Mother drops a tiny spider every other summon (`smallMinion`, now for
  `Summon::Drop` too). A plain bite, no poison (no antidote needed on the first levels). Its own darker tint.
* Tests: `tests/unit/spider_climbers_test.cpp`, `tests/scenarios/spider_climbers.txt`, play script.

## Play-test

`bin/game --play tests/play/spider-climbers.txt`

* Go down the left ladder (col 7), let the spiders see you, go back up and left: the camel spider and the tiny
  spiders climb after you, back to the camera, legs on the rungs.
* Go down again, cross to the right ladder (col 29, its top lit) and go up: they stay below the light.
* The tiny spiders: darker than the camel spider, two club blows each, a weak bite.
