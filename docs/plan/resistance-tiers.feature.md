# Resistance tiers: immune, weak, vulnerable

Status: implemented 2026-11-09, waiting for the user's play-test. Was: draft 2026-11-09, size L, needs nothing first.

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
* The amethyst rings move on this ladder (user, 2026-11-09; replaces today's "share of the way to NORMAL",
  `cutResistances`, and the 20 / 40 / 80 % of commit aff43332). A ring moves each rate a share of the way to the
  next tier up: copper 10 %, silver 20 %, gold 50 % (the sky iron of
  [sky-iron-rings](sky-iron-rings.feature.md): 100 %, a whole tier). The step is the gap to the next tier:
  `rate + (next - rate) * share`; a share over 100 % goes on into the tier after.
  | Rate | Copper | Silver | Gold | Two gold |
  |---|---|---|---|---|
  | tough 25 | 27.5 | 30 | 37.5 | 50 (resists) |
  | resists 50 | 55 | 60 | 75 | 100 (normal) |
  | normal 100 | 105 | 110 | 125 | 150 (weak) |
  | weak 150 | 155 | 160 | 175 | 200 (vulnerable) |
  * Normal and weak rates go up too (not today: the amethyst only lifts resistances).
  * Two rings: the shares add up (gold + silver 70 %), up to 200 % (two sky irons, two tiers); vulnerable is the top,
    never passed.
  * Immune: copper to gold leave it alone; only a sky iron moves it (to tough).
  * The rates are whole percents today (`Resistances`): keep the halves (tenths, or the damage worked out from the
    unrounded rate) so 37.5 stays 37.5; the implementer's choice.
  * `RingEffect::AMETHYST_PERCENT` {10, 20, 50}, `AMETHYST_CAP` 200 (in percent of a tier). The ring card ("Cuts 80% of
    resistances" today) and its sum text: "a tenth / a fifth / half a tier toward more damage" or plainer; the
    journal note `note.ring-amethyst`; the glossary ring row. The unit tests of `ring_accessories_test.cpp` (the
    scarab hit, the sums).
* Hits for 0: no "no damage" chip-damage minimum on an immune type (check `resistedDamage` and the minimum-1 rules),
  a mixed hit still does its other types' share. A visible cue (a "immune" float or the dull hit sound) if one
  exists for tough; else none.
* Unit tests: the word per rate, `bossResist` keeps immune, the amethyst table above, the amethyst skips immune, the Anubis boss takes 0 fire.

## Implementation notes

* Halves kept: `Resistances` holds `float` rates (37.5 stays 37.5), the damage worked out from them, rounded once.
* `amethystResist` (was `cutResistances`) walks `RESIST_TIERS`; `RingBonus::amethystPercent` (was `resistCutPercent`).
* Immune: no copper, silver or gold ring moves it, two gold rings neither (the draft: "only a sky iron moves it").
  The sky-iron move is left to [sky-iron-rings](sky-iron-rings.feature.md).
* A hit for 0 (only immune types): no blood splash; the journal status says "immune to fire". No other cue.
* Ring card: "Resistances 50% of a tier weaker"; sum: "resistances 70% of a tier weaker", "a tier", "two tiers".
* `validKinds`: a common monster is never immune, a boss may be.

## Play-test

* `bin/game --play tests/play/resistance-tiers.txt`: a brazier at the start (E: fire arrows), two gold amethysts worn.
* Walk up, the Anubis guard steps out of the wall; fire arrows on it: it burns, the status says "barely hurt by fire".
* Fire arrows on the Anubis boss: only the stab share hurts, it never catches fire, the status says "immune to fire";
  journal (J): FIRE = immune.
* Inventory, rings tab: an amethyst card says "Resistances 50% of a tier weaker", the sum "a tier weaker".
