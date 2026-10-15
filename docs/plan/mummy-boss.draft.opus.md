# Mummy boss

Status: draft 2026-10-15, ready (needs nothing first). From the user: a boss for the mummy, and the Anubis boss
summons Anubis guards instead of mummies, so every boss's minions are its kin.

## Today

* Bosses (`src/world/monster_kinds.cpp`): each summons its kin, except the Anubis boss (kin `MonsterAnubis`, minions
  `MonsterMummy`, `Summon::Coffin`, 2..4 alive, every 2 s, cap 10). The mummy has no boss.
* Boss levels: 5 Khepri, 10 Shezmu, 15 scorpion queen, 20 Apep, 25 Sobek, 30 Anubis. All taken.
* Mummy: `Locomotion::Entombed`, fists (`MUMMY_MIX`), `MUMMY_RESIST`, threat 5, model `tools/blender/models/mummy.py`.
* Anubis guard: threat 10, faster than the player, the sceptre bolt (`RangedRules`, `BOLT_RED`).

## Idea

### Mummy boss (new kind)

* A mummified pharaoh. `kin = MonsterMummy`, minions mummies with `Summon::Coffin` (taken over from the Anubis boss).
* Entombed like its kin: lies in a large sarcophagus and climbs out when the player comes near.
* **Melee:** a staff, swung like the Anubis's sceptre. Damage mix mostly blunt (e.g. 80 blunt / 20 pierce).
* **Ranged:** a bolt from the staff head (`RangedRules`, release frame measured from the model like
  `ANUBIS_RELEASE*`). Its own look, not the Anubis's red: e.g. a dusty green-black curse bolt (a new `Rgb`).
  `rangedCurses`: Weakness (the mummy's melee curse), stronger. The base mummy gets no ranged attack.
* **Stats:** threat about 15, between Sobek (14.5) and the Anubis boss (16). Start from HP ~2000, damage ~120,
  attack ~1500 ms, Reckless, `trapDamagePct` 10, poison immune, `bossResist(MUMMY_RESIST)` (no weakness, as every
  boss). Fire stays its soft spot as far as `bossResist` allows. Tune with the monster-balance sim if it exists by then.
* **Model:** a variant of `mummy.py` (shared build code, not a copy): a head taller, a staff in the hands (a
  was-sceptre or a crook), a gold mask or a nemes headdress, its own texture. Clips: idle, walk, rise (from the
  sarcophagus), staff swing (melee and ranged, like the Anubis), death. Steps: [modeling.md](../modeling.md).
* **Glyph:** a free one, picked by the implementer.
* **Name / journal:** an Egyptian-flavoured name (implementer's choice, see glossary "Egyptian names"); label
  `mummy boss`. Note e.g. "He was buried with his servants. They were not asked."
* **Placement:** lvl29 for now (a temporary home; the campaign gets extended and re-planned later, after the hippo
  and the other monsters). Give lvl29 a boss room and boss gates; `bin/levelcheck` passes with no warnings. A boss
  on a level that is not a 5th gives a heka like any other (by campaign level); fine for now.

### Anubis boss: guards as minions

* `boss.minion = MonsterAnubis`. A guard is twice a mummy's threat and outruns the player: fewer and slower, e.g.
  1..2 alive, every 4 s, cap 4. Balance in play.
* Summon: coffins do not fit statues. A new `Summon` (e.g. `Plinth`: a statue on a stand at the arena edge steps
  down) or reuse `DigOut` (rises from the floor). Implementer's choice; a new one is preferred if cheap.
* Update the Anubis boss's `description`, `note` ("The dead rise at his word" goes to the mummy boss), the threat
  comment ("plus its mummies") and the mummy's comment ("the Anubis boss's minion").
* lvl30's boss room: coffins placed for the boss's mummies go, the level still passes the checker.

## Open (for the implementer)

* Plinth summon or DigOut.
* Name, glyph, bolt colour.
* Does lvl29's difficulty need trimming with a boss added (it has 11 guards already)?

## Tests

* Unit (`tests/unit/`): the mummy boss summons mummies from coffins; the Anubis boss summons guards; the mummy boss's
  bolt applies its curse; kind table checks (kin, minion) still pass.
* Scenario with screenshots: the mummy boss rising from its sarcophagus, its staff swing and bolt; the Anubis boss
  with guards. A `tests/play/mummy-boss.txt` for the play-test (`level 29`).
