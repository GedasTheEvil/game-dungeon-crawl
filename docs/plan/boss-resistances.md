# Bosses have no weakness

Status: done 2026-10-07, to be play-tested. Split off [monster-balance](monster-balance.draft.md).

## Rule

From the user: a boss is always stronger than its common kin.

* A boss has no weakness. Where its kin is weak (`WEAK`) to a damage type, the boss takes it normally (`NORMAL`).
* A boss never resists worse than its kin, for any damage type, and for poison once monsters can be poisoned
  ([monster-poison](monster-poison.draft.md)).
* Boss HP is not changed for now, though a weak spot doubled the damage: the fights are harder. Retune later with
  [monster-balance](monster-balance.draft.md) if needed.

## Done

`KINDS` (`src/world/monster_kinds.cpp`): each boss row has a `kin` and its resistances are `bossResist(kin's)`, its
kin's with every `WEAK` made `NORMAL`. A `static_assert` fails the build for a boss without a kin, with a `WEAK`, or
with a resistance worse than its kin's.

Blunt, slash, pierce:

| Boss | Kin | Before | Now |
|---|---|---|---|
| Boss scarab | scarab | NORMAL, RESISTS, WEAK | NORMAL, RESISTS, NORMAL |
| Vampire bat | bat | WEAK, RESISTS, TOUGH | NORMAL, RESISTS, TOUGH |
| Anubis boss | Anubis guard | NORMAL, RESISTS, WEAK | NORMAL, RESISTS, NORMAL |
| Scorpion queen | scorpion | WEAK, NORMAL, RESISTS | NORMAL, NORMAL, RESISTS |
| Apep | cobra | RESISTS, WEAK, NORMAL | RESISTS, NORMAL, NORMAL |
| Sobek | crocodile | NORMAL, RESISTS, NORMAL | the same |

## To test

* Each boss fight (lvl5, 10, 15, 20, 25, 30) still winnable at the expected player level, with no best weapon.
* The journal's creature pages show the new resistances.
