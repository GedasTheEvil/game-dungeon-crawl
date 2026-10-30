# Monster and weapon balance

Status: implemented 2026-10-26 (steps 2-5, [Done](#done-2026-10-26)), waiting for the user's play-test:
`bin/game --play tests/play/monster-balance.txt` (a generated serpent temple level with the player the sim puts
there; other zones: the `level`, `xp` and `gear` lines from the table in [Done](#done-2026-10-26)). Merged 2026-10-05
from three drafts: monster strength (2026-10-01), monster XP tuning, and weapon ranges and HP balance. The weapon reach
review is done and verified in play 2026-09-30 (see [Reach](#reach-done)). Refined 2026-10-09: see
[Decided](#decided-2026-10-09).

Needs first: [desert-chapter](solved/desert-chapter.md) and [campaign-zones](solved/campaign-zones.md) (balance
against the final zones, their monster sets and the 40-60 level count), and every monster and boss draft (user, 2026-10-20: balance comes
after all monsters and bosses are in): [giant-bat-screech](solved/giant-bat-screech.md),
[giant-cobra-blinding-spit](solved/giant-cobra-blinding-spit.md),
[giant-rat-pack-call](solved/giant-rat-pack-call.md), [scarab-dung-ball](solved/scarab-dung-ball.md),
[camel-spider-boss](solved/camel-spider-boss.md), [worm-boss](solved/worm-boss.md),
[baboon](solved/baboon.md), [babi-boss](solved/babi-boss.md). A new monster or boss
draft joins this list.

Zones ([campaign-zones](solved/campaign-zones.md#zones-decided-2026-10-21)): about 54 levels; the High Priest moves from
lvl34 to about lvl18, crocodiles from 7-9 to 28 on, Sobek before Seth. Balance against that table.

Levels: [desert-chapter](solved/desert-chapter.md) put five desert levels at 21-25 (Seth's oasis at 25); the
levels named 21-30 below (play-test entries before 2026-10-20) are 26-35 now: Sobek on lvl30, the Anubis boss on lvl35.

Names: the game now shows Egyptian names ([egyptian-names](solved/egyptian-names.md)): Anubis guard = Anubis living statue, Anubis boss = Anubis, boss scarab = Khepri, vampire bat = Bat of Shezmu, mimic = Chest of Set, worm = Sand worm, plant = Thorn acacia. Code ids stay.

## Idea

* Some monsters are too weak for their place in the campaign, others too strong. Review every monster type's threat
  and readjust its stats so each one is a real fight at the levels it appears on.
* Now that monsters and the player have real hitboxes ([monster hitboxes](solved/monster-hitboxes.md)), fights may be
  easier than before (big monsters are hit at their near edge, not their centre). The weapon reach is reviewed; monster
  HP is not yet.
* Some monsters give too much XP for how hard they are to kill. Nerf them, so XP follows the real danger.

The walk speed change ([fixed-timestep.md](solved/fixed-timestep.md), 1.0 tiles/s) is done, so monster speeds can now
be tuned against the final player speed.

## Decided (2026-10-09)

Order of work:

1. **The sim cut** ([sim-library](solved/sim-library.md), done 2026-10-09), so the leap AI below is
   unit tested.
2. **Close the gap:** the giant scarab walks at 3 (from 2). It and the giant rat leap (`Locomotion::WalkJump`) at the
   player when 2-3 tiles away on the same row with a clear way, 3 s between leaps. The giant scarab's
   dung ball ([scarab-dung-ball](solved/scarab-dung-ball.md)) takes 0.6-1 tile between the boxes (Khepri's up to
   1.1; cut from the draft's 4-6 to the view, about 1.8 tiles either side of the player): the leap keeps out of that
   band, from farther (it closes in to the ball's range). HP and damage only if that is not enough. The ball adds to
   the giant scarab's and Khepri's threat (`MonsterKind::threat`, left as it was).
   Done 2026-10-26: the pounce (`PounceRules`, `Dungeon::pounce`): from 2-3 tiles between the boxes, no wall, pit or
   trap between, not from half water (the player cannot jump there either), 3 s apart. The rat lands in bite reach,
   the scarab at 0.8 tiles, in its ball's band, so the windup follows the landing. The landing follows a player who
   steps towards it (never in them). Khepri (the boss rule): 2-3.5 tiles, 2.5 s apart, lands at 1.05, and walks at
   3.5 (from 3) to stay faster than the giant scarab. Tests: `tests/unit/pounce_test.cpp`.
3. **Spear** damage 20 -> 26: the longest melee reach, still behind the sword (35). Done 2026-10-26.
4. **XP, hand-tuned suspects:** plant 1000 -> 500, mimic 1500 -> 900 (they never move, the player picks the fight);
   Anubis stays 10000 (600 HP, 55 damage since the buff). No formula. Bosses keep their hand-set XP. Done 2026-10-26.
5. **Strength by a sim pass:** score every monster against the player's level and gear on the levels it appears on
   (tank in melee, kite with a bow, run past), with the player level from the XP sum per level. Propose a table of
   changes to the user before touching `KINDS`.

Recheck the XP sum per level after steps 4 and 5. Balance comes before the level rebuild (user, 2026-10-21):
[denser-levels](denser-levels.md) builds on these values and rechecks the sum after each batch.

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
| Sobek | 6 | 1600 | 70 | 1300 | 25000 | boss lvl30 (was lvl25), charges (x2) |
| Seth | 5 | 2000 | 80 | 1500 | 30000 | boss lvl25 ([hippo-boss](solved/hippo-boss.md)), charges, storm roar |
| Anubis boss | 4.5 | 2400 | 140 | 1300 | 30000 | boss lvl35 (was lvl30), buffed 2026-10-06 |

Boss minions get their XP from `Dungeon::MinionXP`.

## The 30-level curve (2026-10-06)

The XP curve is steeper past player level 25 (`levelXP`, `src/world/progression.cpp`). Summing every monster's XP
per level (no respawns, no riddles), the player starts lvl15 at about level 35, lvl20 at 43, lvl25 at 49, lvl30 at 55
and ends at about 60 (HP 50 + 12 per level, armour +1 every 5 levels, Might +1 every 8). Before, 30 levels would
have made them level 200+. Needs a playthrough.

## Playthrough reference data

From the user's current playthrough. The curve above is the model; these are the real numbers.

| Date | Map level | Player level | Max HP | Might | Armor | Weapon | Dmg shown | Notes |
|---|---|---|---|---|---|---|---|---|
| 2026-10-09 | 18 | 42 | 738 | 21 | 18 | Epsilon axe lvl 1 (base 55, 30 blunt / 70 slash) | 76 | crocodiles (110 HP) fall in 3 hits |
| 2026-10-10 | 23 | 50 | 1101 | 34 | 32 | Epsilon axe | 100 | the Anubis guard is hardly a threat |

Against the model (2026-10-09 entry):

* Player level 42 on lvl18 is a little ahead of the curve: about 39-40 there (35 at lvl15, 43 at lvl20).
* Max HP 738 is far above the curve's 50 + 12 x 41 = 542: with potions of life and / or a health amulet, add about 36%.
  The curve's "about 700 HP at level 55" (the Anubis and Anubis boss tuning) is already reached at level 42.
* Might 21 and armour 18 are far above level-only gains (about +5 might, +8 armour). Might and armour potions and
  amulets add most of it. Monster damage tuned against "11 armour at level 55" hits softer than planned.
* Crocodile: 3 hits at 76 shown. The scutes resist the axe's slash share, so a hit does about 40-75. Placed for
  levels 7-9 but still met on lvl18, where it is no threat.
* For step 5 (the sim pass): use the real gear growth (potions, amulets), not level-only stats. Ask for more data points
  (lvl20 Apep, lvl25 Sobek, lvl30).

Against the model (2026-10-10 entry):

* Player level 50 on lvl23 is ahead again: the curve gives about 46-47 there (43 at lvl20, 49 at lvl25).
* Max HP 1101 against the curve's 50 + 12 x 49 = 638: +73% (was +36% on lvl18). Gear growth outpaces the levels.
* Might 34 and armour 32: level-only gains are about +6 and +10. From lvl18 to lvl23, +13 might and +14 armour.
* Armour is subtracted from each hit (`playerHitDamage`, at least 1). At 32, every monster with 33 damage or less hits
  for 1: all common monsters but the Anubis (crocodile and giant cobra 26, mummy 20, giant scarab 12 ...). The
  checker's threat and the campaign's late levels count on them.
* **Anubis guard** (600 HP, 55 melee every 1.1 s, bolt 50 every 3 s; blunt weak, slash resisted): the axe deals
  100 x (0.3 x 2 + 0.7 x 0.5) = 95 a hit, about 7 hits. Its melee does 55 - 32 = 23, its bolt 18: about 48 hits to
  kill this player, about 160 HP lost in the fight (15%). Tuned for 15 blows on "700 HP, 11 armour at level 55"
  (44 a hit); now it needs three times that.
* Bosses at armour 32: queen 45 -> 13, Apep 60 -> 28, Sobek 70 -> 38 (charge x2: 108), Anubis boss 140 -> 108.
* For step 5: armour is the lever, not HP. Damage tuned against the curve's armour (about 11) is wiped out by the real
  32. Options to weigh in the sim pass: monster damage up on the late levels, armour as a percentage cut instead of a
  flat one, or fewer armour sources (potions, amulets).

## Sim pass (2026-10-26)

`bin/balance table --gear play` ([development.md](../development.md)): every zone's monsters and its boss against the
player halfway through the zone (the boss: at its level), in the real sim, three seeds. The player: the level at
today's pace (the same share of the way through as on today's 35 levels, whose XP sum, riddles counted, matches the
playthrough: 42 on lvl18, 49 on lvl23), the gear of the playthrough (by player level), the weapon with the hardest hit
of those found by then; up to 4 large health potions in a boss fight, 1 otherwise. Tank: up to it, swinging. Kite:
backs off below 1.2 tiles, shoots, cornered at the wall it fights. Run past: sprints to the far end of the hall.

Found:

* **The common monsters stop being a fight from the web halls on.** From player level ~15, every one costs 0-3% of
  max HP in melee: armour cuts their hits to 1, and the player's HP grows x25 over the campaign (62 -> 1627) while
  their stats stay. Even with no armour, a giant cobra would cost a level 46 player about 6%. Exceptions: the Anubis
  guard (130% on its first meeting at player level 27, 5% at 63) and the baboon (20-25%, a long fight).
* Early (zones 1-2) they are fights: plant 16%, mimic 32%, giant rat 36%; the preview crocodile kills a level 8 player.
* Bosses (tank): Khepri kills a level 4 player, the Bat of Shezmu a level 11 one, the High Priest (moved from lvl34 to
  lvl18) a level 30 one; the Mother of Webs 115%, the queen 49%, Apep 44%, Seth 80%, Rerek 31%, Babi 287% (4
  potions), the Anubis boss 126%. Sobek in his pool: melee never reaches him, the bow takes 204%.
* With the generated supplies (`--gear gen`: the potions on the generated zone levels, much more armour and life) the
  late bosses fall apart: Seth 27%, Rerek 8%, the Anubis boss 44%.
* Fixed on the way: a slow giant scarab or Khepri followed a player backing off at 1.2 tiles, out of the ball's band
  and short of the pounce's 2 tiles: the bow killed Khepri for nothing. Their pounce now starts at 1.2.
* XP: the target campaign generated (`bin/balance curve`: today's levelgen, a ramp of difficulty 1-10) gives 45-55
  monsters on a full level, today's give about 31: the player would end at level 79. The monster XP needs x0.26 (or
  the curve steeper, or fewer monsters) to end at about 60 as today. For [denser-levels](denser-levels.md).
* Levelgen builds no zone level under about 400 open cells: the draft's 100-150 for the first levels does not
  generate. For [denser-levels](denser-levels.md).
* Might adds to each hit: late the flint knife (8, every 400 ms) deals the most per second, at a tenth of a tile.

So a table of KINDS changes alone cannot make each monster a fight on all its levels: a monster in several zones
(the giant cobra in 6, 7, 10 and 12) is tuned for one of them. The options:

* **A, depth scaling:** a monster's HP and damage grow with the level it is on, past its home (its first zone), by
  the player's expected growth there; XP with the HP. KINDS keeps the home values, tuned in the home zone with the
  sim; bosses (one level each) stay hand-tuned.
* **B, armour as a share:** a hit loses armour / (armour + K) of its physical part instead of armour points (K about
  50: armour 10 takes 17%, 44 takes 47%). No cliff where a hit drops to 1, and the gear's spread matters less. Alone it
  does not answer the HP growth.
* **C, the table only:** raise the late monsters' damage over the armour and their HP, keep the rules; a monster after
  its first zone is filler.

## Done (2026-10-26)

The user picked **B + C** (2026-10-26): armour as a share, then the table; no depth scaling, so a monster after its
first zone is easy filler.

* **Armour as a share** (`ARMOR_HALVES` = 50, `playerHitDamage`): a hit's physical part times 50 / (50 + armour).
* **The table** (`KINDS`): each monster's HP and damage (its ranged, roll and roar damage by the same factor) tuned
  with `bin/balance tune` / `home` against the player halfway through its first zone (`ZONE_FAUNA`): a weak one about
  6% of the player's HP in a 3 s melee fight, plant, worm, ushabti, lotus, cobra 10% in 4 s, the giants and the mimic
  20% in 6 s, the hippo and the cursed mimic 30% in 8 s. Two anchors are not the first zone: the Anubis guard is tuned
  for the necropolis (about 30%: the late zones' own monster; in the priests' quarter, one rare guard, it kills in
  melee but a sprint gets past it), the crocodile for the serpent temple (the bat galleries' preview wade: below).
* **Bosses** against the player of their level with up to four large health potions: two or three potions (120-200%
  of max HP) in about 30 s; fewer minions where the stronger kin made the fight (most to 1-3 alive).

| Monster | HP | Damage | Was | | Boss | HP | Damage | Minions alive | Was |
|---|---:|---:|---|---|---|---:|---:|---|---|
| Scarab | 14 | 4 | 10 / 2 | | Khepri | 400 | 8 (ball 12) | 2-3 | 320 / 40 (28), 3-5 |
| Bat | 8 | 4 | 8 / 3 | | Bat of Shezmu | 350 | 25 | 2-3 | 400 / 80, 2-4 |
| Plant | 30 | 4 | 30 / 5 | | Mother of Webs | 800 | 10 | 2 | 520 / 26, 2-4 |
| Mimic | 50 | 6 | 40 / 10 | | High Priest | 1700 | 35 (bolt 80) | 1 | 2000 / 120 (100), 2-4 |
| Worm | 200 | 14 | 30 / 9 | | Scorpion queen | 1800 | 30 | | 700 / 45 |
| Giant rat | 250 | 5 | 60 / 8 | | Apep | 1900 | 70 (spit 11) | 2-3 | 1100 / 60 (15), 2-4 |
| Giant bat | 40 | 28 | 40 / 10 | | Sobek | 1400 | 50 | 1 | 1600 / 70, 1-3 |
| Camel spider | 330 | 5 | 26 / 5 | | Seth | 2500 | 65 (roar 30) | 1 | 2000 / 80 (45), 1-2 |
| Giant scarab | 340 | 15 (ball 11) | 90 / 12 (10) | | Rerek | 3400 | 190 | | 2200 / 75 |
| Mummy | 480 | 100 | 150 / 20 | | Babi | 1500 | 45 (coals 30) | 2 | 2200 / 110 (45), 2-4 |
| Ushabti | 130 | 55 | 40 / 9 | | Anubis | 2400 | 190 | | 2400 / 140 |
| Anubis guard | 1300 | 140 (bolt 70) | 600 / 55 (50) | | | | | | |
| Scorpion | 150 | 30 | 14 / 3 | | | | | | |
| Giant scorpion | 570 | 44 | 70 / 12 | | | | | | |
| Cobra | 330 | 50 (spit 10) | 35 / 6 (2) | | | | | | |
| Giant cobra | 690 | 85 (spit 12) | 160 / 26 (5) | | | | | | |
| Crocodile | 350 | 50 | 110 / 26 | | | | | | |
| Hippo | 585 | 91 | 280 / 28 | | | | | | |
| Lotus | 250 | 90 | 50 / 7 | | | | | | |
| Cursed mimic | 880 | 90 | 120 / 20 | | | | | | |

Rat, giant rat's call, baboon, egg cluster: unchanged (the rat and the baboon already on target).

The player of each zone in the sim (play-test: `xp`, then `gear ARMOUR MIGHT HP%`, `level gen:3:D:ZONE`):

| Zone | Player level | XP | Gear | | Zone | Player level | XP | Gear |
|---|---:|---:|---|---|---|---:|---:|---|
| tomb_entrance | 2 | 1000 | 0 0 0 | | cisterns_of_sobek | 46 | 788559 | 19 24 63 |
| bat_galleries | 8 | 15246 | 0 0 0 | | seths_desert | 49 | 1058223 | 21 27 71 |
| web_halls | 15 | 40233 | 0 0 0 | | land_of_sokar | 52 | 1392657 | 23 30 78 |
| priests_quarter | 27 | 98162 | 5 7 17 | | lake_of_fire | 55 | 1800106 | 26 33 85 |
| scorpion_nest | 32 | 160831 | 9 11 29 | | necropolis | 59 | 2471700 | 29 36 95 |
| serpent_temple | 39 | 367053 | 14 18 46 | | hall_of_judgment | 63 | 3309148 | 32 40 105 |

Left open, for the user and [denser-levels](denser-levels.md):

* **Today's 35 levels no longer balance:** the monsters are tuned for the zones of the rebuild (scorpions of 150 HP on
  today's lvl3-6, crocodiles of 350 on lvl7-9). Play-test on generated zone levels until the rebuild.
* **Khepri** is tuned for a level 6 player with the sword; the paced player of lvl4 is level 4 with the club, and
  she kills them. The first zone must bring the player there (or she needs retuning with the rebuilt zone).
* **The preview crocodile** of the bat galleries kills a level 8 player: drop it, or put it where the wade can be
  skipped.
* **The late zones' fauna** (camel spiders, worms, giant scorpions, mummies, giant cobras in zones 8-12): filler after
  their first zone, as C says; only the Anubis guard, the baboon and the bosses are fights there.
* **XP** is unchanged: it no longer follows the new strengths (a scorpion of the scorpion nest gives 450, an ushabti
  of the zone before 900). With the rebuild's monster counts: the XP sum per level (`bin/balance curve`).
* The checker's `threat` values are unchanged (levelgen's difficulty target rests on them).
* Might adds to each hit: late the flint knife has the most damage a second (a tenth of a tile's reach). Not changed.

## The kite after ranged-weaker-than-melee (2026-10-30)

The tables above were tuned with the old kite: endless arrows, at any range. Plan
[ranged-weaker-than-melee](solved/ranged-weaker-than-melee.md) gave the ranged weapons a few shots (back one
every 2.5 s), a minimum range and softer hits (the self-bow 12 -> 7). The sim's kite now backs off to the bow's
minimum range plus 0.3 tiles, shoots while it has shots, waits for them out of reach, and fights in melee at the
wall. `bin/balance table` before -> after (kite only; tank and run past did not change):

* Bow-only kills (`b`) got rare and slow: plant 14 -> 60 s, worm 14 -> 83 s, mummy 71 -> 82 s (no longer bow
  only), the giant scarab 7 -> 18 s and the cobras, crocodiles and baboons now need the melee finish.
* Worse: the hippo 15% -> 102% (cisterns) and 7% -> 67% (Seth's desert): it charges through the minimum range. Seth
  "no kill" (was 145% in 110 s). The Anubis boss 198% -> 264%, the scorpion queen 92% -> 115%, the baboon 0% in
  2 s -> 20-26% in 48 s.
* Better: Sobek in his pool 150% -> 107%, Babi 175% -> 141% (the kite keeps out of their reach while it waits).
* Unchanged: the flyers the kite never killed (bats), and every tank fight. No monster only the bow can reach
  became unkillable: Sobek's pool fight is shorter, the plants in the campaign all have floor beside them.

## Monster strength

* Go through every type in `KINDS`: speed, HP, damage, attack interval, XP. Compare against the player's level
  and gear at the levels it spawns on (`level_gen.cpp` picks, `docs/levels.md`).
* For each, check three player tactics: tank it in melee, kite it with the bow, run past it.
  A monster that loses to all three at its own levels is too weak.
* Verify with scenarios (`tests/scenarios/`) and a playthrough.

Example: the **giant scarab** (levels 5-10) is slow (speed 2) and easy to kite: the player backs off, shoots it with
the bow and it never reaches them. Easy to tank in melee too. The **giant rat** had the same problem and is now faster
than the walk ([giant-rat-speed.md](solved/giant-rat-speed.md)). The fix for both: step 2 of
[Decided](#decided-2026-10-09).

## HP balance

Not changed: needs a playthrough with the new reach. Retune monster HP (`KINDS` in
`src/world/monster_kinds.cpp`): replay lvl5 (boss scarab, 320 HP) and a giant rat level with each weapon.

Old numbers, at weapon level 1, no Might, from before
[damage types](solved/damage-types-and-resistances.md). Since then the club deals 10 and the spear 20 (26 planned,
step 3), weapons grow per level at their own rate, and each monster takes each damage type at its own rate:

| Weapon | Damage | ms per attack | Damage per s |
|---|---:|---:|---:|
| Club | 9 | 900 | 10 |
| Sword | 35 | 550 | 64 |
| Spear | 15 | 750 | 20 |

* The sword dealt 3x the spear's damage per second and 6x the club's. The spear's reach does not make up for that:
  hence the spear's 26 (step 3) before touching monster HP.
* Boss scarab: 320 HP is ~5 s of sword swings, 16 s with the spear. Its bite (40 every 900 ms) kills a level 8
  player (134 HP) in ~3.6 s standing still, so the fight depends on potions, armour and backing off.

## XP tuning

XP follows the new threat.

* Found in play: the mummy gave too much at 4500 and was cut to 2500 (lvl11, beaten with a sword,
  [solved/mummy-minion-monster.md](solved/mummy-minion-monster.md)). Others are likely off too.
* Not taken (2026-10-09, hand-tuned instead): score each monster from HP, damage per second and mobility (the
  checker's `monsterThreat` is a start), and set its XP
  from that score.
* Suspects: the plant and the mimic (they never move, so the player picks the fight): cut in step 4. The Anubis keeps
  10000 (600 HP since the buff).
* Check the effect on the level curve: the player level when reaching lvl5 / lvl10 bosses is used in the boss tuning
  ([solved/boss-rooms.md](solved/boss-rooms.md)). Rerun a full playthrough or a scenario that sums the XP per level.

## Boss resistances

Split off to [boss-resistances](solved/boss-resistances.md) (done): no boss has a weakness. Boss HP is left as it is for now.

## Open questions

* Which monsters to nerf, by how much: the sim pass (step 5).
* The attack damage mix per monster group ([monster-attack-damage-types.md](solved/monster-attack-damage-types.md)),
  if it plays wrong.

Settled on the way:

* Bosses keep their XP (2026-10-09).
* A crocodile in half water is a harder fight than on land: the player wades at half speed, cannot sprint, jump or
  back off. But by the first crocodile the player has a ranged weapon and can shoot it before it closes in. Its
  `monsterThreat` in the checker score gets x1.1 when it spawns in half water (decided 2026-10-08; done 2026-10-26
  for every swimmer, the hippo too: `SWIMMER_IN_WATER_THREAT_FACTOR`). A wall in the
  checker's walker was dropped: killing it opens the way, as with any monster.

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
* Changes that move the sim pass (step 5): [anubis-speed](solved/anubis-speed.md),
  [anubis-ranged-attack](solved/anubis-ranged-attack.md), [monster-climbers](solved/monster-climbers.md) (threat
  x1.05), [coward-flee-ranged](solved/coward-flee-ranged.md), [crocodile-hold-bite](solved/crocodile-hold-bite.md) (in,
  threat unchanged: short, only in water; the crocodile's half-water weight may want it), the cobra spit cut to 1.2 /
  1.5 tiles (2026-10-09).
* New monsters and bosses to fit in: [scorpion-queen-boss.md](solved/scorpion-queen-boss.md) (the Anubis boss
  must stay stronger), [apep-serpent-boss.md](solved/apep-serpent-boss.md),
  [longer-campaign.md](solved/longer-campaign.md).
* Traps still check the player's point (`Trap::Hurt`), not the player box: left out on purpose in
  [trap-and-font-bugs.md](solved/trap-and-font-bugs.md). Revisit only if trap hits feel off.
