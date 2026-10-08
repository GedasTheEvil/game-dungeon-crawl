# Monster and weapon balance

Status: draft. Merged 2026-10-05 from three drafts: monster strength (2026-10-01), monster XP tuning, and weapon
ranges and HP balance. The weapon reach review is done and verified in play 2026-09-30 (see [Reach](#reach-done));
everything else is open.

## Idea

* Some monsters are too weak for their place in the campaign, others too strong. Review every monster type's threat
  and readjust its stats so each one is a real fight at the levels it appears on.
* Now that monsters and the player have real hitboxes ([monster hitboxes](solved/monster-hitboxes.md)), fights may be
  easier than before (big monsters are hit at their near edge, not their centre). The weapon reach is reviewed; monster
  HP is not yet.
* Some monsters give too much XP for how hard they are to kill. Nerf them, so XP follows the real danger.

The walk speed change ([fixed-timestep.md](solved/fixed-timestep.md), 1.0 tiles/s) is done, so monster speeds can now
be tuned against the final player speed.

## Current values

From `KINDS` (`src/world/monster_kinds.cpp`). Speed, HP, damage, ms between attacks, XP.

| Monster | Speed | HP | Dmg | Attack ms | XP | Note |
|---|---|---|---|---|---|---|
| Scarab | 4 | 10 | 2 | 600 | 300 | |
| Rat | 9 | 12 | 2 | 400 | 300 | |
| Bat | 5 | 8 | 3 | 800 | 400 | |
| Plant | 0 | 30 | 5 | 800 | 1000 | does not move |
| Worm | 1 | 30 | 9 | 1000 | 1200 | |
| Mimic | 0 | 40 | 10 | 800 | 1500 | does not move, leaves a chest |
| Giant bat | 4 | 40 | 10 | 800 | 1800 | |
| Giant rat | 24 | 60 | 8 | 700 | 2000 | leaps, faster than the walk |
| Giant scarab | 2 | 90 | 12 | 1000 | 2500 | leaps |
| Mummy | 2.5 | 150 | 20 | 1600 | 2500 | slow to wake, telegraphed |
| Boss scarab | 3 | 320 | 40 | 900 | 6000 | boss |
| Anubis | 3 | 600 | 55 | 1100 | 10000 | buffed 2026-10-06 for the 30 levels |
| Vampire bat | 4 | 400 | 80 | 800 | 12000 | boss, heals |
| Crocodile | 7 | 110 | 26 | 1100 | 2400 | swims |
| Scorpion | 10 | 14 | 3 | 900 | 450 | weak poison |
| Cobra | 6 | 35 | 6 | 1100 | 1200 | medium poison, spits |
| Giant cobra | 8 | 160 | 26 | 1300 | 2600 | medium poison, spits |
| Giant scorpion | 9 | 70 | 12 | 1000 | 2000 | medium poison |
| Scorpion queen | 5 | 700 | 45 | 1100 | 15000 | boss lvl15, strong poison |
| Apep | 6 | 1100 | 60 | 1200 | 20000 | boss lvl20, burrows |
| Sobek | 6 | 1600 | 70 | 1300 | 25000 | boss lvl25, charges (x2) |
| Anubis boss | 4.5 | 2400 | 140 | 1300 | 30000 | boss lvl30, buffed 2026-10-06 |

Boss minions get their XP from `Dungeon::MinionXP`.

## The 30-level curve (2026-10-06)

The XP curve is steeper past player level 25 (`levelXP`, `src/world/progression.cpp`). Summing every monster's XP
per level (no respawns, no riddles), the player starts lvl15 at about level 35, lvl20 at 43, lvl25 at 49, lvl30 at 55
and ends at about 60 (HP 50 + 12 per level, armour +1 every 5 levels, Might +1 every 8). Before, 30 levels would
have made them level 200+. Needs a playthrough.

## Monster strength

* Go through every type in `KINDS`: speed, HP, damage, attack interval, XP. Compare against the player's level
  and gear at the levels it spawns on (`level_gen.cpp` picks, `docs/levels.md`).
* For each, check three player tactics: tank it in melee, kite it with the bow, run past it.
  A monster that loses to all three at its own levels is too weak.
* Verify with scenarios (`tests/scenarios/`) and a playthrough.

Example: the **giant scarab** (levels 5-10) is slow (speed 2) and easy to kite: the player backs off, shoots it with
the bow and it never reaches them. Easy to tank in melee too. Decided 2026-10-01 (numbers refined when the work
starts):

* Faster: it walks faster (3?), so the player cannot outwalk it.
* Leaps more: it uses its leap (`Locomotion::WalkJump`) to close the gap on the player whenever it can, not only to
  cross pits and traps.
* More HP or damage only if speed and the leap are not enough.

The **giant rat** had the same problem, now faster than the walk:
[giant-rat-speed.md](solved/giant-rat-speed.md). Its leap to close the gap is still open, together with the giant
scarab's.

## HP balance

Not changed: needs a playthrough with the new reach. Retune monster HP (`KINDS` in
`src/world/monster_kinds.cpp`): replay lvl5 (boss scarab, 320 HP) and a giant rat level with each weapon.

Numbers to start from, at weapon level 1, no Might (before
[damage types](solved/damage-types-and-resistances.md): since then the club deals 10 and the spear 20, weapons grow per
level at their own rate, and each monster takes each damage type at its own rate):

| Weapon | Damage | ms per attack | Damage per s |
|---|---:|---:|---:|
| Club | 9 | 900 | 10 |
| Sword | 35 | 550 | 64 |
| Spear | 15 | 750 | 20 |

* The sword deals 3x the spear's damage per second and 6x the club's. The spear's reach does not make up for that:
  consider more spear damage (for example 25-30) before touching monster HP.
* Boss scarab: 320 HP is ~5 s of sword swings, 16 s with the spear. Its bite (40 every 900 ms) kills a level 8
  player (134 HP) in ~3.6 s standing still, so the fight depends on potions, armour and backing off.

## XP tuning

XP follows the new threat.

* Found in play: the mummy gave too much at 4500 and was cut to 2500 (lvl11, beaten with a sword,
  [solved/mummy-minion-monster.md](solved/mummy-minion-monster.md)). Others are likely off too.
* Score each monster from HP, damage per second and mobility (the checker's `monsterThreat` is a start), and set its XP
  from that score.
* Suspects: the plant and the mimic (they never move, so the player picks the fight), Anubis (10000 vs 350 HP).
* Check the effect on the level curve: the player level when reaching lvl5 / lvl10 bosses is used in the boss tuning
  ([solved/boss-rooms.md](solved/boss-rooms.md)). Rerun a full playthrough or a scenario that sums the XP per level.

## Boss resistances

Split off to [boss-resistances](solved/boss-resistances.md) (done): no boss has a weakness. Boss HP is left as it is for now.

## Open questions

* Which monsters to nerf, by how much?
* Should the bosses keep their XP (they are the payoff)?
* A crocodile in half water is a harder fight than on land: the player wades at half speed, cannot sprint, jump or
  back off. But by the first crocodile the player has a ranged weapon and can shoot it before it closes in. Weight its
  `monsterThreat` in the checker score x1.1 when it spawns in half water (decided 2026-10-08). A wall in the checker's
  walker was dropped: killing it opens the way, as with any monster.
* The attack damage mix per monster group ([monster-attack-damage-types.md](solved/monster-attack-damage-types.md)),
  if it plays wrong.

## Reach (done)

Each weapon has a role: the club short and heavy, the sword quick with medium reach, the spear the longest melee
reach (it can hit a monster before its bite lands), the bow at range.

Measured at the hit frame (`tests/scenarios/weapon_reach.txt`, giant rat, debug boxes `hitboxes on` / F3), from the
player's box edge, in tiles. A walker stops and bites at a gap of 0.1 (`MONSTER_BITE_REACH`).

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

## Related

* [solved/trap-walking-monsters.md](solved/trap-walking-monsters.md), [solved/boss-rooms.md](solved/boss-rooms.md).
* New monsters and bosses to fit in: [scorpion-queen-boss.md](solved/scorpion-queen-boss.md) (the Anubis boss
  must stay stronger), [apep-serpent-boss.md](apep-serpent-boss.md),
  [longer-campaign.md](longer-campaign.md).
* Traps still check the player's point (`Trap::Hurt`), not the player box: left out on purpose in
  [trap-and-font-bugs.md](solved/trap-and-font-bugs.md). Revisit only if trap hits feel off.
