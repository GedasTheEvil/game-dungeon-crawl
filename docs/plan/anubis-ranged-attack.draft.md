# Anubis ranged attack

Status: draft 2026-10-09 (idea, not decided). From the user: the Anubis gets a ranged attack, range 14, damage 50,
cooldown 3 s.

## Today

* The Anubis guard (`MonsterAnubis`, `src/world/monster_kinds.cpp`): 600 HP, melee 55 every 1100 ms, mix 80% blunt /
  20% pierce (`ANUBIS_MIX`), reckless, levels 13-30. The Anubis boss (`MonsterAnubisBoss`) shares the model: 2400 HP,
  melee 140.
* The only monster ranged attack is the spit (`SpitRules`, `Dungeon::Venom`, the cobras): from afar along its row it
  stops and spits a glob. `range` is in tiles between the hitboxes (2.5 and 3), `cooldownMs` 3500 / 3000; nearer than
  `MONSTER_BITE_REACH` it bites. The decisions are in `canSpit` (`src/entities/monster_ai.cpp`).

## Idea

| | Value |
|---|---|
| Range | 14 in the weapons' units (tenths of a tile, `WeaponDef::range`): 1.4 tiles, between the throwing stick (12) and the javelin (15). Decided 2026-10-09. As a `SpitRules::range` (whole tiles): 1.4f |
| Damage | 50, of the Anubis's attack mix (or its own mix, see Open) |
| Cooldown | 3 s |

* Reuse the spit machinery: a generic ranged attack (`SpitRules` renamed / widened) with no poison for the Anubis.
* Projectile and look: open. Ideas: a thrown was-sceptre bolt, a beam / bolt of light from the sceptre, a hurled
  spear (pierce). A new attack clip on the Anubis model (Blender, [../remodeling.md](../remodeling.md)), like the
  cobra's spit clip with its `release` and `mouthY`.
* The player can dodge or block it like the venom glob (whatever `Dungeon::Venom` allows today), and it hits walls.
* Journal note: the Anubis's line mentions the ranged attack.

## Open

* The guard only, or the boss too (with its own numbers)?
* Damage type: the melee mix (80 blunt / 20 pierce) or e.g. 100% pierce for a thrown spear?
* Melee and ranged together: it shoots from afar and still hits 55 up close, as the cobras bite up close?
* Balance: the Anubis is already tuned for "about 15 blows" on a level 55 player
  ([monster-balance.draft.md](monster-balance.draft.md)). Check its threat (`threat` 8) and the sim with the new attack.
* `levelcheck` does not model monster attacks; nothing to change there, likely.
* Tests: a scenario in `tests/scenarios/` (an Anubis shoots from range, the cooldown holds, up close it hits in melee).
