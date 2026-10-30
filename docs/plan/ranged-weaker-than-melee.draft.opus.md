# Ranged weapons hit softer than melee

Status: draft 2026-10-30 (user). Needs nothing first.

## Problem

The ranged weapons are too strong next to the melee ones. They shoot from afar with no ammo limit, yet some hit
harder than their melee twin: the throwing stick (14) beats the club (10), the javelin (30) the spear (26), the
self-bow (12) the dagger (8).

## Rule

Soft rule (user): a ranged weapon deals about **a third** of the base damage of its melee twin, the melee weapon of
the same type. "About": round to whole numbers, and the implementer may nudge a value where balance needs it (note
why in the table comment). Write the rule down where the weapons are (`ITEMS`, `src/world/items.cpp`) and in the
glossary (`range`/`ranged weapon` row or a new one).

## Twins and proposed numbers

`WeaponDef::damage` today (growth %, `generatedFrom`):

| Ranged | today | twin (why) | twin's damage | proposed |
|---|---|---|---|---|
| Throwing stick | 14 (20%, 4) | club (the same stick, blunt) | 10 (40%, 0) | 3, growth 40% like the club |
| Javelin | 30 (20%, 4) | spear (the same pole, pierce) | 26 (20%, 3) | 9 |
| Sling | 10 (20%, 0) | club (blunt, from the start) | 10 | 3 |
| Self-bow | 12 (20%, 0) | dagger (pierce, from the start) | 8 (30%, 0) | 3 |
| Composite bow | 22 (20%, 7) | duckbill axe (80% pierce, from level 7) | 50 (20%, 7) | 17 |

The bows and the sling have no melee look-alike: their twin is the melee weapon of the same main damage type and the
same depth (`generatedFrom`). The implementer may pick other twins with a reason.

Twins with a different `growthPercent`: give the ranged one the twin's growth, so the third holds at every weapon
level (`weaponDamage`, `item_bag.cpp`).

## Side effects to handle

1. **Low numbers.** 3 damage rounds coarsely per level (`std::lround`: 3 → 4 → 4 → 5 ...). Check the levels 1-5
   values stay strictly rising, or the inventory card shows a level-up with no gain. If not, raise the growth a little
   rather than the base.
2. **Loot grades** (`WEAPON_GRADES`, `loot.cpp`) sort the weapons by base damage. After the cut, every ranged weapon
   becomes the weakest grade, and a weapon chest's "one weapon of a lower grade" changes. Keep today's order:
   grade a ranged weapon by its twin's damage (or a new `WeaponDef::grade`; implementer's choice). Same check for
   levelgen's weapon chests (`level_gen.cpp`, `generatedFrom`).
3. **Monster balance.** The balance sim kites with the bow (`tools/balance/balance.cpp`, `Tactic::Kite`,
   `bestBow`); plan [monster-balance](monster-balance.md) used that tactic for its tuning. Run `balance table`
   before and after, and report the kite column. Expected and fine: kiting gets ~3x slower. Not fine: a monster that
   only the bow can reach (Sobek in his pool, flyers out of melee reach, monsters on ledges) becomes unkillable or a
   slog. Check each of those; where one is, give that fight a fix (lower HP of that monster, a reachable spot) rather
   than raising the bow back. List them in the report.
4. **Campaign levels.** Anything placed as "shoot it from here" (a target, a lever, a monster behind water) must
   still work: `bin/levelcheck content/levels/lvl*` clean, and a look at levels with ranged-only fights.
5. **Skills on top**: flaming arrows (40% of the shot's damage as fire) and Sharp eye scale with the new base; no
   change needed, just confirm they still read as worth taking.
6. **Unit tests** that pin a ranged weapon's damage (`tests/unit/`: `reach_test`, `skills_test`, `power_hit_test`,
   `world_*`, `scen_*`, ...) get the new values; fights that now need more shots get more shots, not old numbers.

## Guard

A unit test for the rule: each ranged weapon's base damage is at most ~40% of its twin's (the twin table lives in the
test, or as a `WeaponDef` field if item 2 adds one). A later weapon edit that breaks the rule then fails loudly.

## Play-test

`tests/play/ranged-weaker-than-melee.txt`: give every weapon, a few monsters of the mid campaign (giant rat,
crocodile, a flyer), so the user can compare club vs throwing stick and spear vs javelin by hits to kill.
