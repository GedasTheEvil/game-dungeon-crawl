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
* **Curses, his speciality:** he curses a lot, more and nastier than any other monster. Each one has a reason a
  priest of the dead would lay it. `CurseEntries` (`src/world/curses.h`) grows from 2 to 3 entries for every kind
  (the user: the cap was only a suggestion; the others leave the third empty, their rolls unchanged). A hit still
  rolls in order and lays the first that takes, one curse a hit (decided 2026-10-15):
  * Melee, the staff (the embalmer's work on the body):
    1. Greater Disease 12%: the rot the natron was meant to stop.
    2. Greater Weakness 12%: his mummies' curse, stronger.
    3. Fatigue 25%: the long sleep of the tomb.
  * Bolt (the spoken curses of the funeral rites):
    1. Insanity 20%: the forbidden words from the Book of the Dead, heard aloud.
    2. Slowness 25%: bound in linen like the dead.
    3. Vulnerability 30%: weighed against the feather, found wanting.
  * About 42% of the melee hits and 58% of the bolt hits lay a curse (the Anubis boss: 30% and 40%). Tune in play.
  * Journal: the moves read like the others ("Curses with a touch of his staff", "Speaks curses from afar"); the
    curse status lines stay the curses' own.
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
* Summon: a new `Summon::Wall` (decided 2026-10-15): the guards step out of the wall that faces the player, like
  statues coming out of the stone. Pick a wall cell of the boss room in front of the player (in their view, the
  wall they look at; the nearest such cell with a free floor cell in front of it, not behind the player), so they
  see it happen. The guard starts inside the wall and slides out onto the free cell over its `riseMs`, stone dust
  and a grinding sound (reuse what exists: rock-fall dust, the coffin lid's sound); it can't be hit or hurt until
  out. No such wall cell (the player in the open): fall back to the nearest wall of the room. The wall cell stays a
  wall (no hole left), the level is not changed.
* Update the Anubis boss's `description`, `note` ("The dead rise at his word" goes to the mummy boss), the threat
  comment ("plus its mummies") and the mummy's comment ("the Anubis boss's minion").
* lvl30's boss room: coffins placed for the boss's mummies go, the level still passes the checker.

## Open (for the implementer)

* The wall emergence's look (dust, a stone tint that fades as it steps out, ...).
* Name, glyph.

## Tests

* Unit (`tests/unit/`): the mummy boss summons mummies from coffins; the Anubis boss's guards come out of a wall
  cell in front of the player and land on the free cell before it, unhittable until out; the mummy boss's bolt is
  purple and lays its curses; a third `CurseEntry` is rolled (the first two miss, the third takes); kind table
  checks (kin, minion) still pass.
* Scenario with screenshots: the mummy priest rising from its sarcophagus, its staff swing and bolt; guards stepping
  out of the wall in front of the player (mid-way and out). A `tests/play/mummy-boss.txt` for the play-test
  (`level 29`; the guards: `level 30`).
