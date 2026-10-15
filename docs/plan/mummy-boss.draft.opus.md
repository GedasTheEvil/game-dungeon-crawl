# Mummy boss (mummy priest)

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

* A mummy priest (not a pharaoh): one of the temple's priests, a curse-caster. `kin = MonsterMummy`, minions
  mummies with `Summon::Coffin` (taken over from the Anubis boss).
* Entombed like its kin: lies in a large sarcophagus and climbs out when the player comes near.
* **Melee:** a staff, swung like the Anubis's sceptre. Damage mix mostly blunt (e.g. 80 blunt / 20 pierce).
* **Ranged:** a bolt from the staff head (`RangedRules`, release frame measured from the model like
  `ANUBIS_RELEASE*`). **Purple** (a new `Rgb`, e.g. `BOLT_PURPLE`; decided 2026-10-15). The base mummy gets no
  ranged attack.
* **Curses, his speciality:** stronger and more varied than any other monster's, that is what makes the priest
  interesting. Today a trigger holds at most two `CurseEntry` (`src/world/curses.h`). Suggestion: melee lays the
  mummy's Weakness at a higher chance (Greater first); the bolt lays the nastier ones (Greater Disease, Insanity,
  Slowness, ...). If two entries per trigger are too few for a varied bolt, raise the cap for the priest or roll one
  curse from a pool (implementer's choice; keep the other monsters' rolls unchanged).
* **Stats:** threat about 15, between Sobek (14.5) and the Anubis boss (16). Start from HP ~2000, damage ~120,
  attack ~1500 ms, Reckless, `trapDamagePct` 10, poison immune, `bossResist(MUMMY_RESIST)` (no weakness, as every
  boss). Fire stays its soft spot as far as `bossResist` allows. Tune with the monster-balance sim if it exists by then.
* **Model:** a variant of `mummy.py` (shared build code, not a copy): a head taller, a priest's staff in the hands,
  priestly touches (a leopard-skin sash over the wrappings, a shaved-head shape or a simple cap, an amulet), no
  royal regalia; its own texture. Clips: idle, walk, rise (from the sarcophagus), staff swing (melee and ranged,
  like the Anubis), death. Steps: [modeling.md](../modeling.md).
* **Glyph:** a free one, picked by the implementer.
* **Name / journal:** an Egyptian-flavoured name (implementer's choice, see glossary "Egyptian names"); label
  `mummy boss`. Note e.g. "He read the curses at the funerals. He still knows them all."
* **Placement:** lvl29 (a temporary home, fine as is; the campaign gets extended and re-planned later, after the
  hippo and the other monsters). Give lvl29 a boss room and boss gates; `bin/levelcheck` passes with no warnings. A boss
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
* Name, glyph.
* The curse set and how the bolt picks one (see Curses).

## Tests

* Unit (`tests/unit/`): the mummy boss summons mummies from coffins; the Anubis boss summons guards; the mummy boss's
  bolt is purple and lays its curses; kind table checks (kin, minion) still pass.
* Scenario with screenshots: the mummy priest rising from its sarcophagus, its staff swing and bolt; the Anubis boss
  with guards. A `tests/play/mummy-boss.txt` for the play-test (`level 29`).
