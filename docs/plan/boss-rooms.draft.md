# Bosses and boss rooms

Status: refined, ready to plan. Start with the boss scarab in lvl5 (see [Order of work](#order-of-work)).

## Boss room and teleporter

* The old `props/columns.md3` model (with its plasma quad, unused since the ladders, see
  [../remodeling.md](../remodeling.md)) comes back as a **teleporter gate**. It links to another set of columns
  elsewhere on the same map, which is the boss room.
* The teleporter faces the camera. The level's entrance/exit gates stand sideways, along the level.
* Use it with the interact action (`E`, middle mouse button), like the other gates.
* The player can go back through the teleporter without killing the boss, but then has no key to finish the level.
* **Use:** plasma on the gate (like the entrance/exit portals), a sound when it is activated, then the player jumps to
  the paired gate. A short fade out/in is optional.

## Boss

* A special monster that spawns **minions** around itself. The minions are weaker creatures.
* Minion count is random within a range set per boss (`min-max`), for example 3-5 small scarabs for the boss scarab.
* **Minion spawning** (mix of pre-placed and summoned):
  * On arrival the room holds `min` minions.
  * While the boss is alive it summons one minion every N s, only while fewer than `max` are alive in the room.
  * A total summon cap per fight (for example 10-15), so a weak player does not face an endless stream.
* **Per-boss configuration** (a boss class, tuned later from playthroughs), for example a row per boss like
  `MONSTER_DEFS`:
  * minion type
  * `min` minions on arrival, `max` alive in the room
  * summon interval: start with 1-2 s per summon
  * total summon cap
  * minion XP while the boss is alive (1) and after its death (50% of normal)
  * Minion XP: 1 XP while the boss is alive (no farming by staying in the room, but the kill still counts).
    After the boss dies, the minions left give 50% of the normal monster's XP (a scarab: 300 -> 150).
    Letting the boss summon up to `max` before killing it is risk for reward, limited by `max`.
  * They appear next to the boss, never behind the player.
  * When the boss dies, summoning stops and the boss gate opens; the minions left stay and fight on.
  * Each boss summons in its own way: scarabs dig out of the sand, bats drop from the ceiling, mummies rise from the
    broken sarcophagi ([statue-and-mummy-decorations.md](solved/statue-and-mummy-decorations.md)).
* One big boss per boss room. In its level it kills the player in about 3-4 hits.
* **Boss gate:** the boss room has its own gate. Killing the boss opens it. Behind it are 2-3 treasure chests and a
  floating key (for example the blue/lapis key) for a gate further on in the level. So the player fights the boss
  for the key instead of just finding it. The boss does not drop the loot itself.
* How often: a boss every 5 levels (see [Campaign](#campaign)).
* **Boss health bar:** one large bar at the bottom centre of the HUD while the boss is alerted, with its name (the
  status box owns the top centre). The normal
  monster bar stays hidden for the boss.
* **Death in the boss room:** game over, like anywhere else. Loading a save game restarts the fight: monsters are not
  saved (they spawn again from their tiles), so the boss comes back with full HP and the summon count restarts.

## Boss candidates (loosely defined)

| Boss | Minions | Notes |
|---|---|---|
| Anubis (final boss) | Mummies ([mummy-minion-monster.draft.md](mummy-minion-monster.draft.md)) | |
| Boss scarab | Scarabs | Like the giant scarab, but faster and stronger, with more HP. |
| Vampire bat (a very large giant bat) | Small bats | Sucks blood: heals itself by part of the damage it deals. |

## Starting values

Tune from playthroughs.

| Boss | Minions (`min`-`max` alive) | Summon every | Total cap | Other |
|---|---|---|---|---|
| Boss scarab | scarab 3-5 | 1.5 s | 12 | |
| Vampire bat | bat 2-4 | 2 s | 8 | heals 30% of the damage it deals |
| Anubis | mummy 2-4 | 2 s | 10 | |

## Campaign

| Level | Boss | Why |
|---|---|---|
| 5 | Boss scarab | Scarabs are native there; lvl5 had an Anubis once. |
| 10 | Vampire bat | Bats and giant bats run through levels 7-9. |
| 15 | Anubis | The finale. Needs the mummy ([mummy-minion-monster.draft.md](mummy-minion-monster.draft.md)). |

Bosses are campaign only; `levelgen` does not place them at first.

Levels 1 to 5 may be rebuilt from scratch. They are hand-made from before the generator and have little variety.
Redesign them as ASCII sources in `tools/level/campaign/` like levels 4 and 6-15: a difficulty curve into 6,
more of the monsters, traps and mechanisms that exist now, and lvl5 built around the boss room.

## Level format and editor

Findings from the code:

* A cell is `type attr value`. The `value` of a Monster tile is unused (always 0). Door gate types 1-4 are taken.
* Monsters are not saved: after a load every Monster tile spawns again when it comes into view. A dead monster's slot
  is reused once 32 are live, and its tile can then spawn again.
* The view is 10 x 6 tiles and at most 32 monsters are live, so the boss room and its minions must fit in that.

Format:

* **Teleporter:** Door tile, new gate type `5`, `value` = pair id. The two Door-5 cells with the same id link to each
  other.
* **Boss:** new monster types (boss scarab 11, vampire bat 12, Anubis boss 13): a row each in `MONSTER_DEFS` plus a
  boss table (minion type, `min`, `max`, interval, cap, XP rules). When the boss dies, its tile is rewritten (for
  example to `Empty`), so it does not come back after a load or a slot reuse. The boss gate state is in the map
  anyway.
* **Boss gate:** Gate tile with lock colour `5` = boss lock (not one of the 4 key colours: lvl15 needs all of them).
  The boss's death opens every boss gate. One boss per level.
* **Editor:** text lines for gate type 5, lock colour 5 and the boss monster types (`tools/editor/tile_info.cpp`),
  icons if needed. `ascii2level.py` legend entries for the ASCII campaign sources.

## levelcheck

* A teleporter is a two-way edge between its pair, with a small path cost. Going back is always possible, so it
  makes no softlock.
* The boss's cell acts like a lever of the boss colour: reaching it opens the boss gates. The boss's threat counts
  in the difficulty score (`monsterThreat`, well above Anubis 8), plus its minions.
* New warnings: a teleporter id without exactly two gates, a boss gate without a boss, a boss without a boss gate,
  more than one boss, a boss room the player can walk into (it should only be reached by teleport).

## Order of work

1. Teleporter gate: model, plasma, sound, jump, format, editor, checker. **Done:** `Dungeon::Teleport`,
   `teleportPartner` (`level.h`), ASCII `O` (pair id 1), `sounds/mechanisms/teleport.wav`
   (`tools/audio/mechanism_sounds.py`), test `tests/scenarios/teleport.txt`. No fade (optional, left out).
2. Boss framework with the boss scarab: boss table, summoning, XP rules, HUD bar, boss gate, death persisted. Put it
   in lvl5. **Done (framework):** `BOSS_DEFS` / `BossRules` (`assets.cpp`, `monster.h`), `Dungeon::updateBoss` /
   `summonMinion` (`dungeon_monsters.cpp`), `BossBar` (`src/ui/boss_bar.cpp`, bottom centre: the status box owns the
   top), boss gate = lock colour `BOSS_LOCK` (5, `gate_boss.png`), `scarab_boss.png` (`scarab.py --boss-texture`),
   ASCII `K` boss scarab / `Z` boss gate, checker warnings, scenario `killboss` / `boss` / `minions`, test
   `tests/scenarios/boss.txt`. Levels 1, 2, 3 and 5 rebuilt as ASCII sources (lvl4 was already
   one and kept); lvl5 is the scarab king's level (`tests/scenarios/lvl5_boss.txt`). Difficulty curve 6.7, 11.2,
   14.3, 15.5, 19.5 into lvl6 at 21. The old lvl1 / lvl2 are test fixtures (`tests/levels/classic1`, `classic2`).
   Boss scarab: 320 HP, 40 damage (a full clear of levels 1-4 gives level 8, 134 HP), checker threat 10.
   Not yet: the dig-out summon effect (minions just appear).
3. Vampire bat in lvl10 (giant bat model, scaled up, darker texture; life steal).
4. Anubis boss in lvl15 once the mummy monster exists.

## Open questions

* The other bosses, beyond the three above: later, once these three (one per 5 levels) are in and played.
