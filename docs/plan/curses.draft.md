# Curses

Status: draft 2026-10-10, refined twice 2026-10-10 (decided except who applies curses, not implemented). From the user.

## Idea

Timed debuffs on the player, shown as small square icons above the HUD panel with the seconds left, like the poison
drops. Several curses can run at the same time. Getting the same curse again restarts its timer (no stacking).

Curses cannot be cured or resisted: no potion, amulet, level up or resistance lifts or blocks them. They only run
out (and end on death).

All curses last the same time: 2 minutes.

First curse: **weakness**, applied by the mummy's hit with a 10% chance.

Grade roll: when a curse is applied, a second roll picks the grade: greater 10%, normal 90%. This holds for every
curse that has a greater grade (so the mummy can give greater weakness).

## Curses

"Strength" is **Might** in the code: `PlayerStats::Damage = CurrentMight() + weaponDamage`.

| Curse | Effect | Greater |
|---|---|---|
| Weakness | -50% Might | -125% Might (Might goes negative) |
| Clumsiness | -20% total damage, all weapons, not poison | -50% |
| Vulnerability | -60% armour | |
| Fatigue | stamina set to 20% of max at the start; no stamina regeneration while it runs (stamina potions still work) | |
| Disease | -30% max HP | -70% |
| Palsy (working name) | ranged and thrown weapons cannot be used (bow, sling, throwing stick, javelin...); melee works. Single grade |
| Insanity ("caveman") | map, journal, inventory and the quick-drink keys locked; the club is put in hand and the weapon cannot be changed | |

Palsy has no greater grade (skip the grade roll). Name ideas: Palsy (hands tremble), Unsteady Aim, Butterfingers,
Fumbling Hands, Withered Grip. Pick one when implementing.

A greater curse and its lesser one are the same curse at two grades (like the poison tiers).

## What is there today

* HUD: `PlayerHud::View` already draws timed icons above the panel: one poison drop per running tier
  (`poisonLeftMs`) and the resistance potion (`resistLeftMs`). Curses go in the same row, with the same seconds
  label. A curse icon needs its own look (e.g. a dark purple frame) so it does not read as a potion.
* Timers: the resistance potion's `resist_left_ms` in `PlayerStats` (advance, end, save line) is the model to copy.
* Might starts at 0, +1 every 8 levels, +1 per might potion (was +2 until 2026-10-10), plus the amulet's. Weapons deal 10..55. So weakness is
  mild (nothing at Might 0); greater weakness bites into the weapon damage. Clumsiness is the strong damage curse.
* The club is the starting weapon, so the player always has one.

## Sketch

* `enum class Curse { Weakness, Clumsiness, Vulnerability, Fatigue, Disease, Insanity }`, per curse a grade and
  `left_ms` in `PlayerStats`; `Curse(kind, grade, ms)` sets the full time. Save line like `RESIST`.
* Weakness / clumsiness: in `Damage()` (and the ranged / thrown damage paths). Clumsiness after Might and weapon are
  added; the venom amulet's poison is not cut. A hit deals at least 1.
* Vulnerability: in `CurrentArmor()`.
* Disease: in `CurrentMaxHP()`; HP keeps its share on start and end, like `Wear(..., keepShare)` for the health
  amulet.
* Fatigue: `RegenerateStamina` skips while it runs.
* Insanity: the input / UI gates for map, journal, inventory, quick drink and weapon switch ask `PlayerStats::Cursed(Insanity)`; a
  status line says why ("Your mind is clouded"). On start, the weapon in hand is remembered and the club put in hand;
  on end, the old weapon comes back.
* Monster side: `MonsterKind` gets `curse`, `curseGrade`, `cursePercent`, so new cursing monsters need no new code.
  Mummy: weakness, 10%. Roll on the gameplay stream (`GameRandom`) on a hit that lands, then the grade roll.
* Boss rule: no boss has `kin = MonsterMummy`. The Anubis boss summons mummies, and they curse like any mummy. A
  monster given another curse: its boss gets the greater grade (or a higher chance).
* Palsy: the ranged / thrown attack input checks `PlayerStats::Cursed(Palsy)`; status line says why ("Your hands
  shake too much to aim"). The weapon stays in hand, only the shot / throw is refused (no ammo spent, no cooldown).
* **Trigger kind:** `MonsterKind` also gets `curseOn` (`Melee` default, `Ranged`). Palsy triggers only from the
  monster's ranged attack (`SpitRules`, see [anubis-ranged-attack.draft.md](anubis-ranged-attack.draft.md)) when the
  bolt hits the player, never from its melee. Anubis guard: 20%. Anubis boss: 40% (same curse, higher chance, per the
  boss rule). So a monster can carry several curse entries (melee and ranged): make `curse` a small list. Needs the Anubis ranged attack first (a POC exists).
* Status box line on start, refresh and end ("You feel weak", "The weakness passes"); journal note on the first
  curse of each kind.
* Glossary rows: curse and each curse name.
* Tests: unit (refresh, two curses at once, each effect, death, save/load), a scenario in `tests/scenarios/` (mummy
  curses, icon with seconds, damage cut, wears off).

## Decided (2026-10-10)

* No cure, no resistance. Same duration for all (2 minutes).
* A hit deals at least 1 damage (greater weakness can push it below).
* Stamina potions work while fatigued.
* Insanity locks the quick-drink keys too ("totally insane"). The club is always owned (starting weapon).
* Grade roll after the curse roll: greater 10%, normal 90%.

* Anubis applies two curses: Vulnerability from its melee hits (chance open), Palsy (guard 20%, boss 40%) from its
  ranged bolt only (2026-10-11).

## Open

1. **Who applies which curse.** Only the mummy (weakness) is set; the rest is defined later. Ideas (each needs a
   boss pairing, per the boss rule):

   | Curse | Candidate | Why |
   |---|---|---|
   | Disease | rat, giant rat (boss: none; a rat king later?) | plague carriers |
   | Fatigue | vampire bat's kin, the bat / giant bat (boss: vampire bat) | drains |
   | Vulnerability | Anubis (boss: Anubis boss), on melee hits | judge of the dead, weighs the heart |
   | Clumsiness | giant scarab (boss: scarab boss) | |
   | Insanity | the [cursed mimic](cursed-mimic.draft.md), a mimic variant for the higher levels (decided) | none |
