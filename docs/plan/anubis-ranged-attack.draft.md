# Anubis ranged attack

Status: draft 2026-10-09, refined 2026-10-09 (decided), POC implemented 2026-10-10 (see [POC](#poc-2026-10-10)); the
clip, its release and a sound still to do. From the user: the Anubis gets a ranged attack, range 14, damage 50,
cooldown 3 s.

## Today

* The Anubis guard (`MonsterAnubis`, `src/world/monster_kinds.cpp`): 600 HP, melee 55 every 1100 ms, mix 80% blunt /
  20% pierce (`ANUBIS_MIX`), reckless, levels 13-30. The Anubis boss (`MonsterAnubisBoss`) shares the model: 2400 HP,
  melee 140.
* The only monster ranged attack is the spit (`SpitRules`, `Dungeon::Venom`, the cobras): from afar along its row it
  stops and spits a glob. `range` is in tiles between the hitboxes (1.2 and 1.5 since 2026-10-09, like the throwing
  stick and the javelin), `cooldownMs` 3500 / 3000; nearer than
  `MONSTER_BITE_REACH` it bites. The decisions are in `canSpit` (`src/entities/monster_ai.cpp`).

## Idea

| | Value |
|---|---|
| Range | 14 in the weapons' units (tenths of a tile, `WeaponDef::range`): 1.4 tiles, between the throwing stick (12) and the javelin (15). Decided 2026-10-09. As a `SpitRules::range` (whole tiles): 1.4f |
| Damage | 50, of the Anubis's attack mix |
| Cooldown | 3 s |

* Reuse the spit machinery: a generic ranged attack (`SpitRules` renamed / widened) with no poison for the Anubis.
* Projectile and look: see [Decided](#decided-2026-10-09).
* The player can dodge or block it like the venom glob (whatever `Dungeon::Venom` allows today), and it hits walls.
* Journal note: the Anubis's line mentions the ranged attack.
* The Anubis boss gets it too, stronger (the boss ability rule in `AGENTS.md`): see [Decided](#decided-2026-10-09).

## Decided (2026-10-09)

* **Damage type:** the melee mix (`ANUBIS_MIX`, 80 blunt / 20 pierce). No own mix.
* **Melee too:** it shoots from afar (gap 0.1-1.4 tiles) and still hits 55 up close, as the cobras do.
* **Look:** a bolt of light from the was-sceptre; no new projectile mesh. A new attack clip: a sceptre thrust with a
  `release` frame (Blender, [../remodeling.md](../remodeling.md)).
* **Boss:** range 2.0 tiles (the sling's 20), damage 125 (scaled like its melee, 140 vs 55), cooldown 2.5 s. Same bolt,
  same mix.

## Open (for the implementer)

* Balance: the Anubis is already tuned for "about 15 blows" on a level 55 player
  ([monster-balance.draft.md](monster-balance.draft.md)). Check its threat (`threat` 8) and the sim with the new attack.
* `levelcheck` does not model monster attacks; nothing to change there, likely.
* Tests: a scenario in `tests/scenarios/` (an Anubis shoots from range, the cooldown holds, up close it hits in melee).

## POC (2026-10-10)

* `SpitRules` is the generic ranged attack: `poison` optional (none for the bolt), `colour` of the glob
  (`VENOM_GREEN`, `BOLT_RED`). Anubis {50, 1.4 tiles, 3 s}, boss {125, 2.0 tiles, 2.5 s}, both red, both the melee mix.
* Look: the venom glob, red (`Dungeon::drawVenoms`). The attack clip stands in for the spit clip, played once
  (`RANGED_CLIPS`, `model_info.h`); `release` 0.5 and `mouthY` 0.6 are guesses. Sound: its attack sound.
* Journal: the spit move reads "Hurls a bolt of red light from afar. A jump clears it." for a spit without poison.
* Tests: `tests/unit/sim_test.cpp` (it shoots from afar, no poison, then comes on to hit), `tests/scenarios/anubis_bolt.txt`
  (screenshots of the bolt in flight).
* Still to do: the sceptre-thrust clip with a release frame (Blender), `release` / `mouthY` from it, a bolt sound,
  maybe a glow instead of the glob; the balance check (`threat`).
