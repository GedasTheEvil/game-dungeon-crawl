# Shared vitals: one part for what the player and the monsters both live through

Status: solved 2026-11-06. Was: draft 2026-11-06, size L, needs nothing first.

## Problem

The player and the monsters live through the same things (traps, poison, fire, water, a knock on the head, a stun),
but each one has its own code for it, so every new hazard is written twice and the two copies drift apart:

* HP: `PlayerStats` (HP, max HP) and `Monster::health` / `MonsterKind::maxHealth`.
* Poison: one `Poison` class (good), but applied by `Player::Poison` / `UpdatePoison` and
  `Monster::TakePoison` / `UpdatePoison`, each with its own ward roll (`PoisonResistPercent` and
  `MonsterKind::poisonResistPercent`).
* Fire: one `Burning` class (good), but `Player::UpdateBurning` and `Monster::UpdateBurning` (the water douse twice),
  and `Monster::Kindle` beside `player->burning.Ignite`.
* Water: `Player::inWater` and `Monster::inWater` (the fire-in-water rule twice).
* Traps: the player's `stats.trap_carry` / `TrapDamage` (the trap ward), the monster's `trapDamageCarry` /
  `trapHit` (`trapDamagePct`); the same "hundredths carried till a whole HP" twice.
* Out of action: the player's `dazeMs` (`Player::Daze`) and the monster's `reelUntilMs` (`Monster::StunHit`).
* Head bump: `Player::HeadBump` and `Monster::HeadBump`.
* Hits: `Player::TakeHit` (resistances, then armour, then fire) and `Monster::takeHit` / `TakeWeaponHit`
  (resistances, fire), two damage paths.

The first game had `Player` derive from `Monster`. Inheritance is not the way back: a monster carries AI state the
player never has (flight, leap, burrow, charge, grab, climb, roll, kiting, links to the player) and the player carries
equipment, skills, jump physics and the camera. What they share is a part, not a parent.

## Plan

Pure refactor, no change in behaviour: every unit test and golden stays as it is (`make check` green, no golden
rewritten). Kind tooling.

* **`Vitals`** (new `src/level/vitals.h` / `.cpp`, no sim dependencies, so unit tests and the checker can use it): what
  a living thing has, owned by value by both `Player` (or `PlayerStats`, the implementer's choice; the save goes
  through it) and `Monster`:
  * HP and max HP (the player's max HP stays computed by `PlayerStats::CurrentMaxHP`: pass it in, or let `Vitals` hold
    a base and `PlayerStats` the bonuses; the implementer's choice).
  * `Poison poison`, `Burning burning`, `bool inWater`, the trap carry (hundredths), the out-of-action timer (daze /
    stun, see below), `TrapHurt trapHurt`.
  * `Resistances resist`, `int armor` (a monster's 0), `int trapSharePct` (the monster's `trapDamagePct`; the player's
    100 minus the trap ward), `int poisonWardPct`, `int fireWardPct` if any (the Shelter of Serket halving stays the
    player's, passed in as a flag).
* **One hit path**: `struct Harm { int hundredths; DamageMix mix; HarmSource source; bool ignoreArmor; int burnMs; }`
  with `HarmSource` = weapon, monster, trap (trap: no XP for a kill; weapon / player poison: the kill is the player's,
  today's `poisonByPlayer` / `burnByPlayer`). `Vitals::Take(const Harm&)`: resistances (in water: `inWaterResist`),
  armour on the non-fire share (`playerHitDamage`'s rule; armour 0 for monsters gives today's monster numbers), the
  trap share for a trap, the carry, at least 1 HP where today's code has it, the fire share ignites. Returns the HP
  lost. `Player::TakeHit`, `Monster::takeHit`, `TakeWeaponHit`, `TrapHit`, `trapHit` become thin calls into it (god
  mode, the sound, the blood, the journal and the XP stay outside).
* **One tick**: `Vitals::Tick(ms, maxHp, wading)`: the poison's damage, the burning (doused in water), the
  out-of-action timer. Returns the HP lost and who caused it. Replaces both `UpdatePoison` / `UpdateBurning` pairs.
* **Out of action**: one timer with two names kept for the reader: the player is dazed (slower, no jump), a monster is
  stunned (stands still). Same storage, same "at least what is left" rule, the boss cooldown stays the monster's.
* **Head bump**: the HP part (`max(HEAD_BUMP_MIN_HP, HEAD_BUMP_MAX_HP_PCT %)`, straight off) once in `Vitals`; the
  daze / stun roll stays with each caller.
* Save: the player's save format may change (old saves can be ignored, AGENTS.md), but a plain move of the same
  fields keeps it.
* Name in code: the plan slug `shared-vitals` at the new header. Glossary row for "vitals".

## Out of scope

Any rule change (the trap formula, blaze on monsters, terrain for the player, monster icons): those are
[same-hazards-for-all](../same-hazards-for-all.feature.draft.opus.md) and
[monster-status-icons](../monster-status-icons.feature.draft.opus.md). This plan makes them one-place changes.

## Open points (implementer's choice)

* Where the player's `Vitals` lives: in `Player` (beside `burning`, `dazeMs` today) or in `PlayerStats` (beside HP and
  poison, which the save already writes). `PlayerStats` is likely less churn.
* Whether `Vitals` holds max HP or takes it per call.

## As built

* `Vitals` (`src/level/vitals.h`) lives in `PlayerStats::vitals` (the save writes its HP and poison as before, the
  format unchanged) and `Monster::vitals`. `trap_hurt.h` moved to `src/level/` for it; the player's `TrapHurt` moved
  from `Dungeon` into the vitals.
* Resistances and armour are not kept in it: the player's change with the amulet, the curses and the blessings, so a
  `Guard` (resistances, the player's armour) is passed per hit. `Vitals::Cost(Harm, Guard)` is the one damage rule;
  `FireOf`, `Ignite`, `TrapShare`, `TakePoison` the others.
* Two ticks, not one: `PoisonTick` and `BurnTick` keep today's order in the update (the poison and the fire run at
  different points of a tick), each owner's `UpdatePoison` / `UpdateBurning` a thin call.
* The out-of-action timer is the monster's GameClock "until" for both (`KnockOut`, `OutLeftMs`); the player's daze
  ran as a per-tick countdown before (the same length, at most a tick apart).
* The old `struct Vitals` of `item_bag.h` (the player as the bag's rules see it) is now `BagUser`.
