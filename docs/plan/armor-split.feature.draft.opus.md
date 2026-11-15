# Armor split into stab, cut and smash, wards shown

Status: draft 2026-11-15, size M, needs first [ui-tooltip](ui-tooltip.feature.draft.opus.md).

## Idea

Armor is one number today (`PlayerStats::Armor`, points: armour / (armour + 50) of a hit's non-fire share,
`playerHitDamage`, `src/level/damage.h`). The ward amulets (Cut Guard etc., `AmuletType::SlashWard`, `BluntWard`,
`PierceWard`) give a hidden percent on top. Split the protection into stab, cut and smash and show the full value of
each, wards included. The bonus is no longer hidden.

## Plan

* **Model (decided)**: armour stays in points with the same curve; each type gets its own armour points:
  `armor[Blunt|Slash|Pierce]`. Every current armour source (the Armor potion, level up, Djed of Ptah, the Shield of
  Ptah, the armour amulet, boons, offering gifts, the vulnerability curse) adds the same to all three, so the change
  in balance is zero. The wards stay a percent resistance, applied first as now.
* `playerHitDamage`: per type, resistance first, then that type's armour share. Fire stays as now (no armour).
* **Shown**: the inventory's stats card ([ui.md](../ui.md)) replaces the single "Armor" with three rows,
  `Stab`, `Cut`, `Smash`, each showing the total damage it takes off in percent: 1 - resist x 50 / (50 + armour).
  Example: a 20% Cut Guard and 10 armour: `Cut 33.3%`. On hover (tooltip): `cut: 20% + 10a` (ward percent, armour
  points; a row without a ward shows only `10a`).
* A fire row, too (the Sun Disc's percent; armour does not act on fire), so every guard is shown.
* Slight redesign of the stats card to fit the rows; same UI look ([ui.md](../ui.md)).
* Saves: an old save's single armour goes into all three. Leaderboard `armor` field: keep one number (the average, or
  drop it; the implementer decides).
* Monsters are unchanged (no armour; they use resistances).
* Glossary: update the armour row.

## Play-test

* `tests/play/armor-split.txt`: some armour, wearing a Cut Guard. Open the inventory: three rows, cut higher than stab
  and smash. Hover cut: `20% + 10a` style text.
* Drink an Armor potion: all three rows go up.
