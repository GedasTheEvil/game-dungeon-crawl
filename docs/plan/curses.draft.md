# Curses

Status: draft 2026-10-10 (idea, not decided). From the user.

## Idea

Timed debuffs on the player, shown as small square icons above the HUD panel with the seconds left, like the poison
drops. Several curses can run at the same time. Getting the same curse again restarts its timer (no stacking).

First curse: **weakness**. The mummy's hit applies it with a 10% chance. It lasts 2 minutes. Effect: the player's
damage is cut by half.

## What is there today

* HUD: `PlayerHud::View` already draws timed icons above the panel: one poison drop per running tier
  (`poisonLeftMs`) and the resistance potion (`resistLeftMs`). Curses go in the same row, with the same seconds
  label. A curse icon needs its own look (e.g. a dark purple frame) so it does not read as a potion.
* Timers: the resistance potion's `resist_left_ms` in `PlayerStats` (advance, end, save line) is the model to copy.
* "Strength" is **Might** in the code: `PlayerStats::Damage = CurrentMight() + weaponDamage`. Might starts at 0,
  +1 every 8 levels, +2 per might potion, plus the amulet's. Weapons deal 10..55.

## Sketch

* `enum class Curse { Weakness, ... }` and `int curseLeftMs[CURSE_COUNT]` in `PlayerStats`; `Curse(kind, ms)` sets
  the timer to the full time (refresh, not add). Advance with the game clock, ends on death, a level up does not lift
  it (open). Save line like `RESIST`.
* Weakness: `Damage()` halves while it runs (see open question 1).
* Monster side: `MonsterKind` gets `curse` + `cursePercent` (like `poisonResistPercent` / the poisoner's tier), so
  later monsters can curse without new code. Mummy: weakness, 10%. Roll on the gameplay stream (`GameRandom`) on a
  hit that lands.
* Boss rule: no boss has `kin = MonsterMummy`. The Anubis boss summons mummies, they curse as any mummy. Nothing more
  to do unless a mummy boss is added.
* Status box line on start, refresh and end ("You feel weak", "The weakness passes"); journal note on the first curse.
* Glossary rows: curse, weakness.
* Tests: unit (refresh, two curses at once, death, save/load), a scenario in `tests/scenarios/` (mummy curses, icon
  with seconds, damage halved, wears off).

## Open

1. **What gets halved.** Halving only Might does nearly nothing: 0 at the start, a few points later against 10..55
   from the weapon. Suggested: halve the whole hit (`(Might + weapon) / 2`). Alternative: halve Might and also cut the
   weapon by a share.
2. **Cure.** Does anything lift it early: the antidote, a level up (it cures poison), a new potion? Or it just runs
   out.
3. **Resistance.** Does the resistance potion / an amulet protect against curses, or are curses a separate kind?
4. **Arrows / thrown weapons.** Does weakness cut ranged damage too (suggested: yes, all player damage).
5. Next curses, to shape the enum: slowness (walk speed), frailty (armour), no regeneration, darkness (torch radius).
