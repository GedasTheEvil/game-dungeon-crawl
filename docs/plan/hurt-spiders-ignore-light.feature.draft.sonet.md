# Hurt spiders no longer fear the light

Status: draft 2026-11-05, size S, needs nothing first.

## Idea

From the user: spiders only avoid light / fire when they are at full health. When hurt, the fear of fire / light no
longer applies.

## Today

* Light shunners (`MonsterKind::shunsLight`): the camel spider, the tiny spider and their boss, the Mother of Webs
  ([camel-spider](solved/camel-spider.md), [spider-climbers](solved/spider-climbers.feature.md)). They never step into
  a lit cell (`Dungeon::Lit`: a flame within `LIGHT_SHUN_CELLS` on the row, or sunlight), flee when light is between
  them and the player, run for the dark when caught in it, and their climb path goes round lit cells.
* Every check reads `mon.Type()->shunsLight` (`src/world/dungeon_walkers.cpp`: `walkerStops`, `wayOutOfLight`,
  `kiteRoom`, `followPath`'s `ClimbAbility::avoids`, the flee with `litBetween`, `updateWalker`).

## Plan

* A monster shuns the light only while at full HP: one helper (e.g. `Monster::ShunsLight()`:
  `type->shunsLight && Health() >= MaxHealth`), used everywhere the code reads `Type()->shunsLight` for behaviour.
  Any damage counts (a blow, an arrow, poison, burning, a trap).
* Hurt, it walks and climbs into lit cells, chases the player through the light, bites in it, and no longer runs for
  the dark. Monsters do not heal, so it stays fearless for the rest of its life.
* Boss (AGENTS.md, Boss abilities): the Mother of Webs follows the same rule; she still douses torches and lamps
  (`douseCells`) whether hurt or not.
* Journal: the ShunLight move text (`content/texts/`, `creatureMoveNote`) gets a line like "Hurt, it forgets its fear
  of the flame."; the editor readme's camel spider paragraph and the glossary's "light shunner" row say it.
* Checker: no change (it does not model the light; the threat stays).

## Open points (implementer's choice, report them)

* A short tell when it turns fearless (a hiss, the idle clip skipped), or none.

## Tests

* Unit tests in `tests/unit/camel_spider_test.cpp` style: a full-HP camel spider stops at a lit cell; the same one hit
  once (e.g. `world.dungeon` hurt or an arrow) walks through the light and bites the player standing in it. The same
  for the tiny spider on a lit ladder top (`tests/levels/climb_tiny`, the torch at the top): hurt, it climbs up.
* A play script `tests/play/hurt-spiders-ignore-light.txt`: the player in a torch's light, a camel spider out of it,
  a bow to hit it from afar.

## Play-test

* Stand in a torch's light with a bow: the spider waits at the edge of the light; shoot it once and it comes through
  the light and bites.
