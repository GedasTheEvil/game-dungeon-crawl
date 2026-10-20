# Monster attack reach

Status: solved 2026-10-20 (play-tested by the user), implemented 2026-10-20. Draft 2026-10-20, from the user's play-test.

Play-test: `bin/game --play tests/play/monster-attack-reach.txt` (a scarab, a giant scarab and a rat come at you; jump
as one opens its jaws).

## Done

* `MeleeReach` per kind (`monster_kinds.h`): slack past the bite reach (0.15 tiles), half-angle (35°), mouth height
  (half the body). Tall walkers (mummy, Anubis, ushabti, their bosses) strike from up high: `HIGH_BLOW` (50°, mouth at
  3/4). `Monster::Reach` makes a boss's 1.5x longer and at least 50° wide; `validKinds` checks a boss's is no smaller
  than its kin's.
* `Monster::Reaches` (`monster_reach.cpp`): the player's box touches the body (no safe spot inside a monster) or the
  cone. Heights without the water basin's sink, as the player's box has none.
* Hit timing: `Attack` starts a bite, `UpdateBite` lands it `MELEE_WINDUP_PCT` (40%) of `attackMs` later. Not 40% of
  the attack clip: the clip loops and is longer than most bite intervals (a rat bites every 400 ms, its clip runs
  ~1.6 s). During the wind-up the facing stays (a jump over it lands behind its back) and a walker follows only forward
  (`FollowBite`): walking away from a faster monster still gets bitten (the giant rat outruns the player), a jump over
  it does not make it walk backwards.
* Grab: the lunge checks `Reaches` the way the grab started (`GRAB_JUMP_CLEAR` is gone). Flyers: the bat's bite checks
  `Reaches` along its flight.
* A miss snaps at the air (the attack sound). The first one jumped writes the field note "Jumping a bite".
* Dodge window: a scarab's bite is dodged by a jump started in about the first 240 ms of its wind-up
  (`tests/scenarios/solved/monster_attack_reach.txt`).
* Open questions decided: cone (as asked), hit at the wind-up, no extra cost for a jump over (it turns once the bite is
  done), no safe spot inside a box (the body check).
* Tests: `tests/unit/bite_reach_test.cpp`, `hold_test.cpp` (jump over the giant scorpion's tell), scenario
  `tests/scenarios/solved/monster_attack_reach.txt` (the miss, screenshots). Bat bites: the existing bat tests in
  `sim_test.cpp`.

## Problem

Jumping over a small monster (giant rat, scarab) still gets the player bitten. The bite checks only the row and the
horizontal gap, not the height:

* `Monster::attackDirection` (`src/entities/monster_ai.cpp`): 0 ("in reach") when the boxes are within
  `MONSTER_BITE_REACH` horizontally, or when the player overlaps the monster, as mid-jump right above it.
* `Monster::sameRow`: the player is within 0.8 tiles vertically. A jump peaks ~0.4 tiles high, so the jumping player
  stays "on the row".
* `Monster::Attack` / `bite()` deal the damage at once, when the attack starts; nothing checks reach at the hit.

## Idea

Each monster attack gets a reach shape, like a weapon's range: a cone from its mouth (or claws, sting) towards its
facing. The bite hits only if the player's box touches the cone at the hit moment; else it misses (no damage, no
poison, no grab, a miss sound / the snap in the air).

* Cone: origin at the mouth (from the model: front edge, mouth height as a share of `TopY - BottomY`), along the
  facing, length = the kind's reach (default `MONSTER_BITE_REACH` plus a bit), half-angle per kind (default ~35°).
  Data in `MonsterKind` (e.g. `MeleeReach { float length; float halfAngleDeg; float originY; }`), defaults so most
  kinds need no entry.
* Small low monsters (rat, scarab, small scorpion, snake on the floor): a low, flat cone. A jump clears them.
* Tall ones (mummy, Anubis, crocodile rearing, bosses): a cone that reaches up, a jump over them does not help (the
  player can't clear them anyway).
* Flyers (`monster_flight.cpp` `bite()`): the same check from their position (bats dive at the player's head).
* Behind it: the player who lands behind a monster is out of the cone until it turns. It turns at its next step
  (`facing`), so jumping over buys a short opening, not immunity.
* Hit timing: check reach at the clip's hit frame, not when the attack starts (default: a short wind-up per kind,
  e.g. 40% of the attack clip). This lets a jump started as the attack begins dodge it, and makes the miss visible.
* Grab (`monster_grab.cpp`) already checks `attackDirection == 0` at the lunge; switch it to the cone too.
* Ranged attacks keep their projectile rules (already jumpable).
* Bosses: same rule, their cones bigger (boss rule, AGENTS.md): longer, taller.
* The player's melee: out of scope, but note the asymmetry; the player hitting a monster below while jumping stays as
  is.
* Journal: optional line when the player first jumps a bite ("It snapped at my heels and caught only air.").

## Open questions (implementer's choice, defaults above)

* Cone vs. simple box (reach + height band): a box is simpler and enough for a 2D side view; the cone matters only
  for tall attackers versus a jumping player. Default: cone, as the user asked.
* Hit-frame timing: changes the feel of every melee monster; may need balance (damage per second drops a little).
* Should a jump over a monster still cost something (e.g. it turns faster, a free attack once landed)?
* Monsters that overlap the player while standing (the player walks into one): cone from the front edge still
  covers them; check no "safe spot" inside a monster's box.

## Tests

* Unit / sim: rat bites the standing player; a jump over the rat (player above its cone) takes no damage; a jump over
  a mummy is still hit; behind the monster: no hit until it turns; flyer bite reaches a standing player; grab misses a
  jumping player; a boss's cone is bigger than its kin's.
* Scenario: jump over a scarab mid-attack, screenshot of the miss.
