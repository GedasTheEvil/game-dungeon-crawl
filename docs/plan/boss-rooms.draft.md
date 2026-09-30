# Bosses and boss rooms

Status: idea, not refined. Collect more detail before planning.

## Boss room and teleporter

* The old `props/columns.md3` model (with its plasma quad, unused since the ladders, see
  [../remodeling.md](../remodeling.md)) comes back as a **teleporter gate**. It links to another set of columns
  elsewhere on the same map, which is the boss room.
* The teleporter faces the camera. The level's entrance/exit gates stand sideways, along the level.
* Use it with the interact action (`E`, middle mouse button), like the other gates.
* The player can go back through the teleporter without killing the boss, but then has no key to finish the level.

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
    broken sarcophagi ([statue-and-mummy-decorations.md](statue-and-mummy-decorations.md)).
* One big boss per boss room. In its level it kills the player in about 3-4 hits.
* **Boss gate:** the boss room has its own gate. Killing the boss opens it. Behind it are 2-3 treasure chests and a
  floating key (for example the blue/lapis key) for a gate further on in the level. So the player fights the boss
  for the key instead of just finding it. The boss does not drop the loot itself.
* How often: a boss every 3 or every 5 levels (not decided).

## Boss candidates (loosely defined)

| Boss | Minions | Notes |
|---|---|---|
| Anubis (final boss) | Mummies ([mummy-minion-monster.draft.md](mummy-minion-monster.draft.md)) | |
| Boss scarab | Scarabs | Like the giant scarab, but faster and stronger, with more HP. |
| Vampire bat (a very large giant bat) | Small bats | Sucks blood: heals itself by part of the damage it deals. |

## Open questions

* The other bosses, beyond the three above.
* Starting values per boss for `min`, `max` and the total cap; tune from playthroughs.
* How the editor and the level format mark a teleporter pair, a boss and its boss gate.
* How `levelcheck` treats a teleporter jump and a key behind a boss gate.
