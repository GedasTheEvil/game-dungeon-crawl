# Baboon rebalance: double its health

Status: draft 2026-11-16, size S, needs nothing first.

## Idea

From the user: at map level 42 the player does 89 damage with the chariot bow, and a baboon of the Lake of Fire
falls in one hit, even without flaming arrows. Double the baboon's hit points, at least 200 after the change.

## Plan

* `MonsterBaboon` in `src/level/monster_kinds.cpp`: `.maxHealth` 110 → 220 (double, above the 200 floor).
* Comment on the entry: "mid HP" no longer fits; say it takes a few bow hits (plan baboon-double-health).
* Babi (`MonsterBabi`, 1500) stays: the change is for the base monster only, not a new ability.
* Check what reads `maxHealth` for balance (threat, XP, level checker budgets): if `.threat` or `.xp` are meant to
  follow HP, raise them in step and say so in the commit; if not, leave them.
* `bin/levelcheck content/levels/lvl*` must stay clean (the Lake of Fire, levels 42-45); fix a level it flags.
* A unit test that the baboon's max health is at least 200 (names the plan).
* Version: DEFAULT +1 (a balance feature).

## Play-test

* `bin/game --level 42`: shoot a baboon with the chariot bow (plain arrows): it survives the first hit, the health
  bar about half down; the second or third arrow kills it.
