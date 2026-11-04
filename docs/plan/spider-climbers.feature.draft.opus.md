# Spider climbers, and a tiny spider

Status: draft 2026-11-02, size L, needs nothing first. From the user.

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
