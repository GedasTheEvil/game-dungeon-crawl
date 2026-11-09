# Resistance tiers: immune, weak, vulnerable

Status: draft 2026-11-09, size M, needs nothing first.

## Idea

The damage-type ladder (`src/level/damage.h`) has four steps today: weak 200, normal 100, resists 50, tough 25.
Nothing sits between normal and double, and nothing takes no damage. The user's new ladder (2026-11-09):

| Taken | Word | Constant |
|---|---|---|
| 200 % | vulnerable | `VULNERABLE` (today's `WEAK`, renamed) |
| 150 % | weak | `WEAK` (new value) |
| 100 % | normal | `NORMAL` |
| 50 % | resists | `RESISTS` |
| 25 % | tough | `TOUGH` |
| 0 % | immune | `IMMUNE` (new) |

First use of immune: the Anubis boss (the final boss, `bossResist(ANUBIS_RESIST)`) is immune to fire (tough today).
Poison is not a damage type: it has its own `poisonResistPercent`, and the boss already has `POISON_IMMUNE`. Nothing
to do there; check the journal says "immune" for both.

## Plan

* `damage.h`: `VULNERABLE = 200`, `WEAK = 150`, `IMMUNE = 0`. Every `WEAK` use today means 200: rename it to
  `VULNERABLE` first (monster tables, `bossResist`, the flaming arrows' rule "whoever resists pierce is weak to fire",
  its unit test, the comments). No monster changes its numbers, bar the Anubis boss's fire. Giving some monsters
  the new 150 step is a separate balance question.
* `resistanceWord`: "vulnerable", "weak", "normal", "resists", "tough", "immune" (rate 0). The journal
  (`content/texts/journal.txt`, `journal_pages.cpp`), the monster status texts (`dungeon_monsters.cpp`), the scenario
  script words (`scenario_script.cpp`), `status_list.cpp`, and the glossary rows (resistance, weakness, hostile
  terrain, flaming arrows, the boss rule "no weak spot" = no vulnerability).
  The curse Weakness and the poison tier "weak" are other things: leave them.
* `bossResist`: caps at NORMAL as now (a boss is neither weak nor vulnerable); an `IMMUNE` rate passes through.
  The Anubis boss: its fire set to `IMMUNE` after `bossResist` (its kin, the Anubis guard, stays tough: the boss is
  stronger, AGENTS.md Boss abilities). Its own fire bolt: no change. A flaming arrow on it: the fire part does 0.
* Immune vs the amethyst rings (`cutResistances`): copper to gold cut a rate below NORMAL toward NORMAL; today an
  immune 0 would become normal under a gold amethyst. Rule: the amethyst does not touch `IMMUNE`
  (the sky-iron amethyst of [sky-iron-rings](sky-iron-rings.feature.draft.opus.md) moves it one tier).
* Hits for 0: no "no damage" chip-damage minimum on an immune type (check `resistedDamage` and the minimum-1 rules),
  a mixed hit still does its other types' share. A visible cue (a "immune" float or the dull hit sound) if one
  exists for tough; else none.
* Unit tests: the word per rate, `bossResist` keeps immune, the amethyst skips immune, the Anubis boss takes 0 fire.

## Play-test

* 
