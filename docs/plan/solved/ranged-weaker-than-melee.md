# Ranged weapons: a few shots, not too close, a little weaker

Status: draft 2026-10-30 (user). Needs nothing first.

## Goal

Make melee worth its risk. Today a ranged weapon wins almost every fight: it hits about as hard as melee (the throwing
stick 14 vs the club 10, the javelin 30 vs the spear 26), shoots without limit and works even at point blank. So the
player kites everything and never takes the melee risk.

Wanted play: ranged is the opener (soften a monster as it comes, pull one out of a group, reach what melee can't),
melee finishes. The player chooses, and the melee risk pays off in a faster kill.

Three changes together; each closes a different hole.

## 1. Shots: a few, coming back one by one

Each ranged weapon holds a few shots. A shot spends one; none left, no shot (a dry click, the HUD count blinks).
They come back one at a time while the player does not shoot that weapon.

| Weapon | Shots |
|---|---|
| Self-bow | 5 |
| Composite bow | 6 |
| Sling | 6 |
| Throwing stick | 3 |
| Javelin | 2 |

* **Recovery:** one shot every ~2.5 s, starting ~2 s after the weapon's last shot (every shot restarts the delay).
  Kiting means shooting, so a kiting player stays low. Optional (implementer's choice): faster while standing still,
  as a bonus, never a must: no waiting in empty corridors.
* **Per weapon**, in the bag (`ItemBag`): switching weapons does not refill. All weapons recover at once, also the ones
  not in hand.
* A new field per ranged weapon (`WeaponDef::shots`) and the recovery times in `gameplay_config.h`. Shots and recovery
  go through one place (a `PlayerStats`-style getter) so the amulets that follow can scale them
  ([amulet-more-shots](amulet-more-shots.md), [amulet-quick-shots](amulet-quick-shots.md)).
* **HUD:** the shots left as pips on the weapon's tile (the quick slot), the recovering one filling up. Full: no pips,
  or dimmed (implementer's choice; it must not clutter melee play).
* Saves: the counts need not be saved; a load starts full (AGENTS.md, Old save games).
* Journal, weapons page: a line on shots and recovery.

## 2. Minimum range

A ranged weapon does not shoot at a monster closer than its minimum range, measured like `WeaponDef::range` (between
the hitboxes, tenths of a tile, a new `WeaponDef::minRange`): bows 8, sling, throwing stick and javelin 5 (halved after the first play-test, 2026-10-30: 15 and 10 felt too large).

* Too close: the shot does not leave (no shot spent), the aim mark turns red / shows "too close", a short dull sound.
  No weaker shot: a clear no reads better.
* Only an aimed monster counts: shooting at a wall, a lever or a target is never blocked.
* The player then steps back or switches to melee. Check the weapon keys switch fast enough that this is a choice, not
  a trap; if the switch is slow, shorten it here.
* Jumpers and chargers close the gap fast: that is the point, no exception for them.

## 3. A little weaker

Soft rule (user): a ranged weapon hits softer than its melee twin, the melee weapon of the same type. Two caps:

* **per hit:** below the twin's base damage;
* **per second:** at most ~70% of the twin's damage per second (base damage / `WeaponMotion::AttackMs()`); the
  dagger is fast, so damage per hit alone misleads.

The rule is a cap: it never raises a weapon. Twins and proposed numbers (damage, attack time, damage per second):

| Ranged | today | twin (why) | twin | proposed |
|---|---|---|---|---|
| Throwing stick | 14, 700 ms, 20/s | club (the same stick, blunt) | 10, 900 ms, 11/s | 5 (7/s), growth 40% like the club |
| Sling | 10, 800 ms, 12.5/s | club (blunt, from the start) | 10, 900 ms, 11/s | 6 (7.5/s) |
| Javelin | 30, 1200 ms, 25/s | spear (the same pole, pierce) | 26, 750 ms, 35/s | 20 (17/s) |
| Self-bow | 12, 1000 ms, 12/s | dagger (pierce, from the start) | 8, 400 ms, 20/s | 7 (7/s) |
| Composite bow | 22, 1300 ms, 17/s | duckbill axe (80% pierce, from level 7) | 50, 900 ms, 56/s | 22, as is |

The bows and the sling have no melee look-alike: their twin is the melee weapon of the same main damage type and the
same depth (`generatedFrom`). Other twins with a reason are fine. A twin with another `growthPercent`: the ranged one
takes the twin's growth, so the caps hold at every weapon level.

Balance target, rather than the exact numbers: a full set of shots takes about a third to a half of a typical
monster's HP in the zone where the weapon is found. Tune the damage and the shot counts against that.

Write the rule as a comment at `ITEMS` (`src/world/items.cpp`) and in the glossary (new rows: `shots`,
`minimum range`; update `range`).

## Side effects to handle

1. **Low numbers.** Small bases round coarsely per level (`weaponDamage`, `std::lround`). Levels 1-5 must stay
   strictly rising, or a level-up shows no gain; raise the growth a little, not the base.
2. **Loot grades** (`WEAPON_GRADES`, `loot.cpp`) sort the weapons by base damage: lower ranged damage changes a chest's
   "one weapon of a lower grade". Keep today's order: grade a ranged weapon by its twin's damage, or a new
   `WeaponDef::grade` (implementer's choice). Check levelgen's weapon chests too (`level_gen.cpp`).
3. **Monster balance.** The balance sim kites with the bow (`tools/balance/balance.cpp`, `Tactic::Kite`, `bestBow`).
   Teach it the shots, their recovery and the minimum range (backing off to shoot, switching to melee when out of
   shots or too close). Run `balance table` before and after and report it. Expected: pure kiting gets much slower or
   fails. Not fine: a monster that only ranged can reach (Sobek in his pool, flyers out of melee reach, monsters on
   ledges) becomes unkillable or a slog. Fix such a fight (that monster's HP, a reachable spot), not the bow. List
   them in the report. Plan [monster-balance](monster-balance.md) tuned with the old kite tactic: note there what
   changed.
4. **Monsters that keep their distance** (the ranged ones, the cowards that flee, plan
   [coward-flee-ranged](coward-flee-ranged.md)): check they can still be killed without endless chasing.
5. **Campaign levels.** Anything placed as "shoot it from here" (a target, a lever, a monster behind water) still
   works: `bin/levelcheck content/levels/lvl*` clean, and a look at the levels with ranged-only fights.
6. **Skills on top:** flaming arrows (40% of the shot as fire, stamina per shot) and Sharp eye scale with the new
   base; check they still feel worth taking. The flaming shot also spends a shot.
7. **Unit tests** that pin a ranged weapon's damage or shoot many times in a row (`tests/unit/`: `reach_test`,
   `skills_test`, `power_hit_test`, `world_*`, `scen_*`, ...): new values; a fight that now needs more shots waits
   for recovery or uses melee, not the old numbers.

## Tests

* Unit: shots spend, run out, come back one by one after the delay; a shot restarts the delay; per weapon (switching
  does not refill); the minimum range blocks a shot at a close monster and spends nothing, never blocks one at a
  wall or lever.
* Unit, the guard: each ranged weapon's base damage is below its twin's, and its damage per second at most ~70% of
  the twin's (the twin table in the test, or the `WeaponDef` field if item 2 adds one).
* Scenario: the HUD pips (full, two spent, one recovering) and the "too close" aim mark, screenshots.

## Play-test

`tests/play/ranged-weaker-than-melee.txt`: every weapon, a few mid-campaign monsters (giant rat, crocodile, a flyer),
so the user can compare: open with the bow, finish with the club or spear; kite with only the bow.

## Implemented (2026-10-30)

* Shots, recovery, the HUD pips, the journal line, the twin caps and the numbers as above. Loot grades: a new
  `WeaponDef::gradeDamage` keeps today's order. No faster recovery while standing still (the optional bonus).
* Minimum range measured as the aim's range is (from the launch point, just ahead of the player, to the monster's
  near edge). The throwing stick's is 7, not 10: with its 1.2 range, 10 left a 0.2-tile window to throw from.
* There is no aim mark in the game: "too close" shows as the status line "Too close to shoot", the weapon slot's red
  blink and a dull knock (`shot_too_close.wav`); no shot left is a dry click (`shot_dry.wav`) and the pips blink.
  A monster that comes too close during the draw stops the shot too, nothing spent.
* The weapon keys switch at once (only not during a swing): no change needed.
* Balance before / after: [monster-balance](monster-balance.md#the-kite-after-ranged-weaker-than-melee-2026-10-30).
