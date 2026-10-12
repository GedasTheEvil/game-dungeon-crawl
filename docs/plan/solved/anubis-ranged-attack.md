# Anubis ranged attack

Status: solved 2026-10-12, play-tested by the user (see [Implemented](#implemented-2026-10-12)). Its scenario is
parked: `tests/scenarios/solved/anubis_bolt.txt`; the behaviour is in `tests/unit/sim_test.cpp`. From the user: the Anubis gets a ranged attack, range 14,
damage 50, cooldown 3 s.

Names: the game now shows Egyptian names ([egyptian-names](egyptian-names.md)): Anubis guard = Anubis living statue, Anubis boss = Anubis, boss scarab = Khepri, vampire bat = Bat of Shezmu, mimic = Chest of Set, worm = Sand worm, plant = Thorn acacia. Code ids stay.

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
* **Look:** a bolt of light from the was-sceptre; no new projectile mesh. The clip: see
  [the clip](#decided-2026-10-12-the-clip) (the melee swing, no new clip).
* **Boss:** range 2.0 tiles (the sling's 20), damage 125 (scaled like its melee, 140 vs 55), cooldown 2.5 s. Same bolt,
  same mix.

## Decided (2026-10-12): spit becomes ranged attack

Spitting is no longer the only ranged attack, so "spit" names the cobra's venom only. The generic machinery is the
ranged attack, in files and code:

* **Clip:** suffix `_spit` -> `_att_ranged`, beside `_att`. `models/monsters/cobra_spit.md3` ->
  `cobra_att_ranged.md3` (`tools/blender/models/cobra.py`: the action `cobra_spit` and its suffix; re-export, or
  rename the file and the script together), `ModelState::Spit` -> `ModelState::AttackRanged`, the clip tables in
  `model_info.h`. The Anubis has no `_att_ranged` clip: `RANGED_CLIPS` maps `AttackRanged` to `_att`
  ([the clip](#decided-2026-10-12-the-clip)).
* **Sound:** `_spit.wav` -> `_att_ranged.wav` (see [the sound](#decided-2026-10-12-the-sound)).
* **Code ids:** `SpitRules` -> `RangedRules`, `MonsterKind::spit` -> `ranged`, `Monster::Spit` / `Spitting` /
  `canSpit` / `spitReleased` -> `AttackRanged` / `AttackingRanged` / `canAttackRanged` / `rangedReleased`,
  `CharacterSound::Spit` / `spitSound` -> `AttackRanged` / `attackRangedSound`, the projectile `Dungeon::Venom` ->
  `Dungeon::MonsterShot` (venom and bolt alike). Names that are about the poison glob itself stay venom
  (`VENOM_GREEN`, the venom drawing). Where: `grep -rlw -i spit src tests tools`.
* **Docs:** the `spit` and `range` rows in `docs/glossary.md` (a new `ranged attack` row; `spit` = the cobra's
  venom kind of it), `docs/remodeling.md` (cobra clip list, sounds), `docs/development.md` (monster fields).
* `RangedRules::mouthY` -> `releaseY` (the sceptre head is no mouth).
* A rename only: no behaviour change. Its own commit, before the clip and the sound, so the build and tests prove it.

## Decided (2026-10-12): the clip

* **No new clip.** The ranged attack plays the melee swing (`anubis_attack`, `_att`, `tools/blender/models/anubis.py`
  `attack_poses`) once: wind-up overhead (frame 8 of 26), the sceptre swings down and forward (strike, frame 12). The
  bolt leaves the sceptre head as it points at the player, mid swing. The wind-up is the player's cue to jump.
* **Release point, measured, not guessed:** in `anubis.py`, a `-- --measure` option (like `cobra.py`'s) prints the
  sceptre head's position per attack frame: its forward reach and its height as a share of the reference clip's
  height (walk frame 0, `referenceTop`). Pick the frame where the head points most nearly forward, between wind-up
  and strike (about frame 10-11, `release` about 0.4); `releaseY` = its height there. Set both for the Anubis and
  the boss (same model) in `monster_kinds.cpp`, replacing the POC's 0.5 / 0.6.
* The bolt starts at the hitbox front (`HeadX`), not at the sceptre's x. If the measure shows the head well ahead of
  or behind that at the release, the implementer may add a forward offset beside `releaseY`; otherwise not.
* Blender 5 headless (AGENTS.md path) for the measure; no re-export, the model files do not change.

## Decided (2026-10-12): the sound

* **File name:** a ranged attack's sound is `<model>_att_ranged.wav`, beside `<model>_att.wav`. Rename the spitters'
  `_spit.wav` with it: `cobra_spit.wav` -> `cobra_att_ranged.wav` (the `character_model.cpp` table,
  `tools/audio/cobra_sounds.py`).
* **Who plays it:** the ranged sound whenever the model has one, not only for a poison spit
  (`monster_ai.cpp`, `Monster::Spit`: today a spit without poison plays the attack sound).
* **The sound, `anubis_att_ranged.wav`, about 0.7 s:** magic, not a weapon. Over it, the Anubis grunt
  (`anubis_att.wav`) at 70% volume, from the start.
  1. Charge, about 0.25 s: a low hum rising in pitch, a faint glassy shimmer. It plays during the thrust, a cue to
     jump.
  2. Release: a short dry crack (a spark, a snapped reed).
  3. Tail, about 0.3 s: a falling hiss and buzz as the bolt leaves.
  Short and dry: it must not sound like the teleporter's smooth rising swell (`teleport.wav`).
* **Made by** a new `tools/audio/anubis_sounds.py`, synthesized like the other `tools/audio/` scripts (fixed seed,
  16-bit PCM mono 22050 Hz). The grunt is a sample in IMA ADPCM, which Python's `wave` cannot read: decode it with
  `ffmpeg -i sounds/monsters/anubis_att.wav -f s16le -ac 1 -ar 22050 -` and mix it in at 0.7.
* **Boss:** the same file (no per-monster pitch in the sound code). The fire bolt later swaps the shimmer for a
  crackle ([anubis-fire-bolt](../anubis-fire-bolt.draft.opus.md)).

## Open (for the implementer)

* Balance: the Anubis is already tuned for "about 15 blows" on a level 55 player
  ([monster-balance.draft.opus.md](../monster-balance.draft.opus.md)). Check its threat (`threat` 8) and the sim with the new attack.
* `levelcheck` does not model monster attacks; nothing to change there, likely.
* Tests: a scenario in `tests/scenarios/` (an Anubis shoots from range, the cooldown holds, up close it hits in melee).

## POC (2026-10-10)

* `SpitRules` is the generic ranged attack: `poison` optional (none for the bolt), `colour` of the glob
  (`VENOM_GREEN`, `BOLT_RED`). Anubis {50, 1.4 tiles, 3 s}, boss {125, 2.0 tiles, 2.5 s}, both red, both the melee mix.
* Look: the venom glob, red (`Dungeon::drawVenoms`). The attack clip stands in for the spit clip, played once
  (`RANGED_CLIPS`, `model_info.h`); `release` 0.5 and `mouthY` 0.6 are guesses (now:
  [the clip](#decided-2026-10-12-the-clip)). Sound: its attack sound.
* Journal: the spit move reads "Hurls a bolt of red light from afar. A jump clears it." for a spit without poison.
* Tests: `tests/unit/sim_test.cpp` (it shoots from afar, no poison, then comes on to hit), `tests/scenarios/anubis_bolt.txt`
  (screenshots of the bolt in flight).
* Still to do: `release` / `mouthY` measured from the swing, the bolt sound (both decided above), maybe a glow
  instead of the glob; the balance check (`threat`). Fire bolt and fireball look, after fire damage:
  [anubis-fire-bolt](../anubis-fire-bolt.draft.opus.md).

## Implemented (2026-10-12)

* Rename (its own commit): clip and sound suffix `_att_ranged` (`cobra_att_ranged.{md3,wav}`), `RangedRules`,
  `MonsterKind::ranged`, `ModelState` / `CharacterSound` / `CreatureMove::AttackRanged`, `Monster::AttackRanged` /
  `AttackingRanged` / `canAttackRanged` / `TakeShot`, `Dungeon::MonsterShot` / `shots` / `shootAtPlayer`, `SHOT_*`,
  `releaseY`. Glossary: a `ranged attack` row.
* Release, measured (`anubis.py -- --measure`): frame 12 of 26 (`release` 12/25), the sceptre head 0.44 of the height
  (`releaseY`), 2.6 half widths ahead of the centre (new `RangedRules::releaseAhead`, default 1 = the hitbox front;
  the cobras keep it). The start never passes the player's near edge (`Monster::TakeShot`). Guard and boss alike
  (`ANUBIS_RELEASE*`, `monster_kinds.cpp`).
* Sound: `tools/audio/anubis_sounds.py` writes `anubis_att_ranged.wav` (1.23 s): the grunt at 70% from the start, a
  0.3 s rising hum with a glassy shimmer, the crack at 0.86 s (frame 12 at 14 fps), a 0.32 s falling hiss and buzz.
  Every ranged attack plays its `_att_ranged` sound.
* Balance: `threat` Anubis 9 -> 10, boss 15 -> 16 (`docs/levels.md`); `./levelcheck levels/lvl*` stays clean. The
  sim pass of [monster-balance](../monster-balance.draft.opus.md) (step 5) weighs the bolt in; play data so far
  (2026-10-10, lvl23) found the guard "hardly a threat", so the bolt goes the right way.
* Not done: a glow instead of the glob (the red ball reads well in the scenario); a scenario for the cooldown.
