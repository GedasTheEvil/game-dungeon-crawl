# Curses

Status: draft 2026-10-10, refined twice 2026-10-10 (who applies which curse decided 2026-10-11), implemented 2026-10-12 (see [Implemented](#implemented-2026-10-12)), waiting for play-test. From the user.

Names: the game now shows Egyptian names ([egyptian-names](egyptian-names.md)): Anubis guard = Anubis living statue, Anubis boss = Anubis, boss scarab = Khepri, vampire bat = Bat of Shezmu, mimic = Chest of Set, worm = Sand worm, plant = Thorn acacia. Code ids stay.

## Idea

Timed debuffs on the player, shown as small square icons above the HUD panel with the seconds left, like the poison
drops. Several curses can run at the same time. Getting the same curse again restarts its timer (no stacking).

Curses cannot be cured or resisted: no potion, amulet, level up or resistance lifts or blocks them. They only run
out (and end on death).

All curses last the same time: 2 minutes.

First curse: **weakness**, applied by the mummy's hit with a 10% chance (greater weakness 2%, see Monster side).

Grades: a greater grade is its own entry on the monster, with its own (lower) chance. No separate grade roll.

## Curses

"Strength" is **Might** in the code: `PlayerStats::Damage = CurrentMight() + weaponDamage`.

| Curse | Effect | Greater |
|---|---|---|
| Weakness | -50% Might | -125% Might (Might goes negative) |
| Clumsiness | -20% total damage, all weapons, not poison | -50% |
| Vulnerability | -60% armour | |
| Fatigue | stamina set to 20% of max at the start; no stamina regeneration while it runs (stamina potions still work) | |
| Disease | -30% max HP | -70% |
| Slowness | walk (and climb) 1 → 0.5 tiles/s, sprint 3 → 1.25 tiles/s (`WALK_SPEED` 1, sprint x3) | |
| Palsy (working name) | ranged and thrown weapons cannot be used (bow, sling, throwing stick, javelin...); melee works. Single grade |
| Insanity ("caveman") | map, journal, inventory and the quick-drink keys locked; the club is put in hand and the weapon cannot be changed | |

Palsy has no greater grade. Name ideas: Palsy (hands tremble), Unsteady Aim, Butterfingers,
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

* `enum class Curse { Weakness, Clumsiness, Vulnerability, Fatigue, Slowness, Disease, Palsy, Insanity }`, per curse a grade and
  `left_ms` in `PlayerStats`; `Curse(kind, grade, ms)` sets the full time. Save line like `RESIST`.
* Weakness / clumsiness: in `Damage()` (and the ranged / thrown damage paths). Clumsiness after Might and weapon are
  added; the venom amulet's poison is not cut. A hit deals at least 1.
* Vulnerability: in `CurrentArmor()`.
* Disease: in `CurrentMaxHP()`; HP keeps its share on start and end, like `Wear(..., keepShare)` for the health
  amulet.
* Fatigue: `RegenerateStamina` skips while it runs.
* Slowness: the walk step (`PLAYER_MOVE_STEP`, `stepHeldWalk`) x0.5 and the sprint factor 3 → 2.5 (so 1.25 tiles/s)
  while it runs. In half water the wade factor applies on top (0.25 tiles/s, no sprint there anyway). `levelcheck`
  keeps assuming no curse (rock falls, rhythm traps are timed against the plain walk).
* Insanity: the input / UI gates for map, journal, inventory, quick drink and weapon switch ask `PlayerStats::Cursed(Insanity)`; a
  status line says why ("Your mind is clouded"). On start, the weapon in hand is remembered and the club put in hand;
  on end, the old weapon comes back.
* Monster side: `MonsterKind` gets `std::array<CurseEntry, 2> meleeCurses, rangedCurses` (max 2 each, 4 in total),
  `CurseEntry { Curse kind; Grade grade; float percent; }`, so new cursing monsters need no new code. Each entry has
  its own chance. On a hit that lands (melee hit, or ranged bolt hit), roll the matching list on the gameplay stream
  (`GameRandom`), strongest entry first; the first success applies, so one curse per hit. No separate grade roll.
  Mummy: melee weakness 10%, greater weakness 2% (tunable; the stronger curse has the lower chance).
* Boss rule: no boss has `kin = MonsterMummy`. The Anubis boss summons mummies, and they curse like any mummy. A
  monster given another curse: its boss gets a higher chance, or the stronger grade.
* Palsy: the ranged / thrown attack input checks `PlayerStats::Cursed(Palsy)`; status line says why ("Your hands
  shake too much to aim"). The weapon stays in hand, only the shot / throw is refused (no ammo spent, no cooldown).
* **Trigger kind:** the two lists are the trigger kind. Palsy sits in `rangedCurses`: it triggers only when the
  monster's ranged attack (`SpitRules`, see [anubis-ranged-attack.draft.opus.md](anubis-ranged-attack.draft.opus.md)) hits the
  player, never from melee. Anubis guard: 20%. Anubis boss: 40% (higher chance, per the boss rule). Needs the Anubis
  ranged attack first (a POC exists).
* Status box line on start, refresh and end ("You feel weak", "The weakness passes"); journal note on the first
  curse of each kind.
* Glossary rows: curse and each curse name.
* Tests: unit (refresh, two curses at once, each effect, death, save/load), a scenario in `tests/scenarios/` (mummy
  curses, icon with seconds, damage cut, wears off).

## Decided (2026-10-10)

* No cure, no resistance. Same duration for all (2 minutes).
* A hit deals at least 1 damage (greater weakness can push it below).
* Stamina potions work while fatigued.
* Slowness (user, 2026-10-11): walk 1 → 0.5, sprint 3 → 1.25 tiles/s. Single grade.
* Slowness comes from melee bites (`meleeCurses`) of the worm (15%), the giant cobra (15%) and Apep (30%, boss of
  the cobra kin, per the boss rule) (user, 2026-10-11; the chances are starting values). Not the plain cobra, not
  the spit.
* Insanity locks the quick-drink keys too ("totally insane"). The club is always owned (starting weapon).
* Grades are separate entries with own chances (replaces the grade roll, 2026-10-11). Each monster: max 2 melee + 2 ranged curse entries.

* Anubis applies two curses: Vulnerability from its melee hits (guard 15%, boss 30%, `meleeCurses`), Palsy (guard 20%, boss 40%) from its
  ranged bolt only (2026-10-11).

## Who applies which curse (decided, user 2026-10-11)

All melee bites (`meleeCurses`) unless marked ranged. Chances are starting values: plain monster lower, its giant
higher, the boss (per the boss rule) highest; a greater grade is its own entry with a low chance.

| Curse | Monster: chance | Why |
|---|---|---|
| Weakness | mummy 10%, greater 2% | |
| Disease | rat 10%; giant rat 15%, greater 3% (no boss has rat kin; a rat king later?) | plague carriers |
| Slowness | worm 15%, giant cobra 15%, Apep 30% (boss of the cobra kin) | coils and sand |
| Fatigue | bat 10%, giant bat 15%, vampire bat 30% (boss of the bat kin) | drains |
| Vulnerability | Anubis 15%, Anubis boss 30% | judge of the dead, weighs the heart |
| Palsy | Anubis 20%, Anubis boss 40%, ranged bolt only (`rangedCurses`) | |
| Clumsiness | giant scarab 15%; scarab boss (Khepri) 30%, greater 6% | |
| Insanity | the [cursed mimic](cursed-mimic.draft.opus.md) 25% | |

## Implemented (2026-10-12)

* Model: `src/world/curses.h/.cpp`: `Curse` (saved by place, append only), `CurseGrade`, `CURSES` (names, status
  lines on start, again and end), `Curses` (per curse a grade and the time left; `Lay`, `Advance`, save line
  `CURSES n kind grade ms ...` after the `HEKA` line, old saves: none), the effect numbers (`WEAKNESS_MIGHT_CUT` ...)
  and the pure helpers `cursedMight`, `cursedArmor`, `cursedMaxHP`, `cursedHit`.
* Effects in `PlayerStats`: `CurrentMight`, `CurrentArmor`, `CurrentMaxHP` and `Damage` go through the helpers, so
  the inventory and the HUD show the cursed values. Rounding: toward zero (`value * (100 - cut) / 100`); a hit deals
  at least 1. `LayCurse` / `AdvanceCurses` / `EndCurses` keep the HP's share of the max HP (disease), fatigue drops
  the stamina to 20% if above (it never raises it) and `RegenerateStamina` skips while it runs. Slowness:
  `WalkCurseFactor` (0.5, on every walk and climb step in `input.cpp`) and `SprintMoveMultiplier` 2.5.
* A greater curse over a running lesser one raises it; a lesser one over a running greater one only restarts the time.
* Monster side: `MonsterKind::meleeCurses` / `rangedCurses` (`CurseEntries`, two each), filled per the table above.
  `Player::Curse` rolls them in order on the gameplay stream after a hit that landed (not dodged, player alive):
  `Monster::bite`, the charge hit (melee list) and the spit / bolt hit in `dungeon_arrows.cpp` (ranged list). The
  journal gets `CreatureMove::Curse` ("Its blows curse: my arm goes weak.").
* Palsy: `tryAttack` refuses a ranged weapon's attack (no shot, no ammo, no cooldown), "Your hands shake too much to
  aim", once per key press. Insanity: the screen keys (inventory, map, journal and the Heka screen too), the quick
  drinks and the weapon keys say "Your mind is clouded"; `Inventory::FollowInsanity` (each tick) puts the club in
  hand and gives the old weapon back at the end. A load during insanity forgets the old weapon (the club stays).
  No monster lays insanity yet: [cursed-mimic](cursed-mimic.draft.opus.md) adds it.
* HUD: the curse tiles right of the poison and resistance tiles, in `Curse` order: stone tile in a purple frame, the
  curse's icon (`hud_icons.png` grew a row: cells 32-39, `Icon::FirstCurse`, `tools/textures/hud_icons.py`), one pip
  lesser, two greater, the seconds under it. Many tiles at once can run under the heka tile (rare).
* Journal: a field note per curse kind (`FieldNote::CurseWeakness` ..., `curseNote`), written the first time.
* Death and `Reanimate` end all curses.
* Tests: `tests/unit/curses_test.cpp` (timers, refresh, grades, save, each effect, lines and notes, roll order, death,
  the monster table, a mummy cursing in the sim, the Anubis bolt's palsy). Scenario `tests/scenarios/curses.txt`
  (command `curse NAME [greater]`, field `curses`): the HUD tiles, palsy, insanity, all worn off after 2 minutes.

Open (play-test): the chances, the numbers, the icons' look.
