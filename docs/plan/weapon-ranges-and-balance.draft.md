# Weapon ranges and HP balance

Status: reach review done and verified in play 2026-09-30 (see [Reach](#reach)); HP retune open, waits for a
playthrough. Follow-up of [monster hitboxes](solved/monster-hitboxes.md), whose bug fix is verified.

## What

Now that monsters and the player have real hitboxes, fights may be easier than before (big monsters are hit at
their near edge, not their centre). Review the reach of every weapon, then retune monster HP.

* Each weapon has a role: the club short and heavy, the sword quick with medium reach, the spear the longest melee
  reach (it can hit a monster before its bite lands), the bow at range.
* Check with the debug boxes (`hitboxes on`, F3) that each reach looks right against the model in the hand.
* Check in play and with scenarios: a small monster (scarab) and a big one (giant rat, boss scarab) against each
  weapon. The player should land hits at the distance where the monster bites; only the spear outreaches the bite.
* Then retune monster HP (`MONSTER_DEFS`, `BOSS_DEFS` in `src/state/assets.cpp`): replay lvl5 (boss scarab, 320 HP)
  and a giant rat level. Not before the reach review.

## Reach

Measured at the hit frame (`tests/scenarios/weapon_reach.txt`, giant rat, debug boxes), from the player's box edge,
in tiles. A walker stops and bites at a gap of 0.1 (`MONSTER_BITE_REACH`).

| Weapon | Tip at the hit | Reach before | Reach now |
|---|---:|---:|---:|
| Club | ~0.1 | 0.4 | 0.2 |
| Sword | ~0.15 | 0.5 | 0.3 |
| Spear | ~0.3 | 0.8 | 0.5 |
| Bow (aim) | | 3 | 3 |

The old reach was 3-4 times the drawn weapon: every weapon hit well before the bite. Now the club and sword reach
end at their tips (a little past, for the arc of the swing), still past the bite gap, so a standing fight always
hits (`monster_hitboxes.txt` passes unchanged). Only the spear clearly outreaches the bite (0.4 past it).

Found on the way: the spear thrusts upwards (`strikeTilt` 70), so its tip passes over a low monster's head (rat,
scarab) while the hit counts. Tilt it down if it looks wrong in play.

## HP balance (open)

Not changed: needs a playthrough with the new reach. Numbers to start from, at weapon level 1, no Might (before
[damage types](damage-types-and-resistances.md): since then the club deals 16 and the spear 26, and each monster
takes them at its own rate):

| Weapon | Damage | ms per attack | Damage per s |
|---|---:|---:|---:|
| Club | 9 | 900 | 10 |
| Sword | 35 | 550 | 64 |
| Spear | 15 | 750 | 20 |

* The sword deals 3x the spear's damage per second and 6x the club's. The spear's reach does not make up for that:
  consider more spear damage (for example 25-30) before touching monster HP.
* Boss scarab: 320 HP is ~5 s of sword swings, 16 s with the spear. Its bite (40 every 900 ms) kills a level 8
  player (134 HP) in ~3.6 s standing still, so the fight depends on potions, armour and backing off.
* Replay lvl5 and a giant rat level with each weapon, then tune `MONSTER_DEFS` / `BOSS_DEFS`.

## Related

* Traps still check the player's point (`Trap::Hurt`), not the player box: left out on purpose in
  [trap-and-font-bugs.md](solved/trap-and-font-bugs.md). Revisit only if trap hits feel off.
