# Camel spider boss: the web spinner

Status: draft 2026-10-20, from the user ("for the remaining bosses, a spider boss makes sense; others remain without
a boss"). Refined 2026-10-22 (open points decided by the user). Needs nothing first. Implemented 2026-10-23, see
[Implemented](#implemented-2026-10-23); play-test: `bin/game --play tests/play/camel-spider-boss.txt`.

## Lore

Egyptian myth has no spider god, and spiders are rare in Egyptian art (Neith is the goddess of weaving, but she is
never a spider). So this boss is a beast, not a god, like the hippo before Seth. The hook comes from the game itself:
the camel spider's journal note ends with "It spins no webs. So who made these?" The decor webs (`DECOR_WEB`) in the
halls are hers. Name: "the Mother of Webs" (user, 2026-10-22); not a god's name.

## Idea

`MonsterSpiderQueen` (working name), kin `MonsterCamelSpider`. Boss rule (AGENTS.md): the camel spider's abilities,
stronger.

* Model: the camel spider's, scaled up (about 2.5x), own texture (darker, banded, a pale sheen); the camel spider's
  clips (user, 2026-10-22).
* **Shuns the light, stronger:** faster than the camel spider (which already outruns the player). The camel spider
  only keeps out of the light; she puts it out: a torch or a lamp within about 2 tiles of her path goes dark when she passes and stays dark
  for good (user, 2026-10-22): the arena darkens as the fight goes on, so the light the player relies on does not last. The braziers stay lit (the player's refuge, and the fire
  arrows' source).
* **Webs:** she spits or lays web on the floor of her row: a cell of web slows the player like the sand fall's slow (a slow, not a hold: user, 2026-10-22); a blade
  cuts it, fire burns it. The decor webs are hers.
* Minions: camel spiders drop from the ceiling (`Summon::Drop`, as the vampire bat's bats), out of the webs.
* Weakness: the camel spider's (blade, fire); `bossResist` drops the weaknesses to normal.
* Journal note answering the camel spider's: "Found the one who makes the webs."

## Placement

Final place (user, 2026-10-21): the web halls, zone 3 of [campaign-zones](campaign-zones.md) (about
levels 10-13), placed by the rebuild ([denser-levels](denser-levels.md)). Until then no campaign slot:
a play script only (`tests/play/camel-spider-boss.txt`), as hippo-boss did at first (user, 2026-10-22).

## Others

The rest stay without a boss (user, 2026-10-20): rats, plants, mimics, the lotus. The worm has its own draft
([worm-boss](worm-boss.md)).

## Tests

* Unit / sim: faster than the camel spider, keeps out of the braziers' light, puts out a torch, the web slows
  and a blade or fire clears it, camel spiders drop round her.
* Scenario with a screenshot: in the dark, a torch going out, a web on the floor.

## Implemented (2026-10-23)

* `MonsterMotherOfWebs` (33, glyph `)`), "Mother of Webs", kin and minion `MonsterCamelSpider`, `Summon::Drop`
  (2 at once, up to 4, every 3 s, 10 a fight). Speed 34 (the camel spider's 28), 520 HP, 26 damage every 0.6 s,
  threat 12. Camel spider model at scale 70 (2.5x), `camel_spider_mother.png` (`camel_spider.py --boss-texture`),
  her own voice (`tools/audio/mother_sounds.py`: `mother_{att,die,wake}.wav`).
* Douse (`MonsterKind::douseCells` = `MOTHER_DOUSE_CELLS` 2): awake, every wall torch and oil lamp within 2 cells of
  her body along her row goes out (`DecorLayout::doused`), with a hiss (`FireHiss`) and its flame dying down
  (`SNUFF_MS`). Reloading the level lights them again (the boss fight starts over too). As a douser she heeds only
  the braziers and sunlight (`Dungeon::shunned`): she puts a torch out before she reaches its light. A light shunning
  boss flees like a coward (`Monster::flees`): a brazier's light between her and the player keeps her out of bow
  range. `validKinds` checks a light shunner's boss shuns light too.
* Webs (`WebRules`, `Dungeon::Web`): she lays them (no spit) on dry floor every 1.2 s, up to 8 whole. The player
  standing in one walks at `WEB_SLOW_FACTOR` (the sand fall's 0.4), not in a jump; "Your feet stick in the web". A
  swing whose main damage is a cut (the short sword, the khopesh, the epsilon axe) cuts the webs in its reach; a fire arrow low over a cell
  and a fire vent's jet burn them. Not saved. Drawn as faint grey threads over the floor (`dungeon_render_webs.cpp`).
* Journal moves `Douse`, `SpinWeb`.
* Tests: `tests/unit/mother_test.cpp`, `tests/scenarios/solved/mother.txt`, `tests/levels/mother`; play script
  `tests/play/camel-spider-boss.txt`.
