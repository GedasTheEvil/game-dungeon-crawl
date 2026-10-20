# Monster attack reach

Status: draft 2026-10-20, ready (needs nothing first). From the user's play-test.

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
