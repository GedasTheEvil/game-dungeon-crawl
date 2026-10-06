# Cobra: a regular monster

Status: implemented 2026-10-06 (not placed in any level yet). Draft 2026-10-06. The minion of [apep-serpent-boss.md](apep-serpent-boss.md), built first as a
regular monster. Placement in the campaign comes later (with the boss or
[longer-campaign.md](longer-campaign.md)).

## Decided (2026-10-06)

* **Egyptian cobra** (Naja haje): sand brown with darker bands, a hood.
* **Idle: coiled.** Lies coiled on the floor, head on the coils, until the player comes near along its row (or hits
  it). Then it rears up, the hood spreads (a rise clip, once), and it hunts like a walker.
* **Bite:** pierce, **medium poison** ([solved/poison-and-antidote.md](solved/poison-and-antidote.md)).
* **Venom spit** at range: a glob of venom flies at the player. A hit poisons (medium) and deals a little damage. It
  flies at chest height: a jump can dodge it. The cobra stands still while it spits.
* **Swimmer:** 125% speed in half water
  ([solved/crocodiles-and-flooded-cells.md](solved/crocodiles-and-flooded-cells.md)).

## Numbers (starting values)

| | Value |
|---|---|
| Speed | 6 |
| HP | 35 |
| Bite | 6 every 1100 ms, pierce, medium poison |
| Spit | 2 damage + medium poison, range 2.5 tiles, every 3.5 s, glob 3 tiles/s |
| XP | 1200 |
| Wake range | 1.8 tiles along the row |
| Resistances | blunt resists (the coils give), slash weak, pierce normal |

## Model

`tools/blender/models/cobra.py`. Clips: move (the reference: slithers, front third raised, hood half open), attack
(strike from the reared pose), die, idle (coiled, head on the coils), rise (idle to move frame 0, hood spreads, once),
spit (hood flared, head jerks forward, mouth open, once; the release frame and the mouth position go to the engine).
Sounds: `cobra_{wake,att,die,spit}.wav`.

## Engine

* `Locomotion::Coiled`: lurks (idle) until woken, plays the rise clip, then walks. Shares the lurk / rise logic with
  the entombed mummy (no coffin).
* Spit: a `SPIT_DEFS` row per spitter (damage, poison tier, range, cooldown); a venom projectile list in `Dungeon`
  next to the arrows, hit-tested against the player's box. A new journal move "Spit".

## Done

* `MonsterCobra` (17, glyph `c`), scale 24, numbers as above (`MONSTER_DEFS`, `SPIT_DEFS`, `POISON_DEFS`,
  `WADING_DEFS`, `RESISTANCE_DEFS` in `src/state/assets.cpp`). Checker threat 3.
* `Locomotion::Coiled` (`COILED_CLIPS`, `COILED_WAKE_RANGE`): the rise clip like the mummy's (`Monster::rises`), turned
  to the player while coiled and rising.
* `ModelState::Spit`, `Monster::canSpit / Spit / TakeSpit`; `Dungeon::Venom` in `dungeon_arrows.cpp` (`VENOM_*` in
  `gameplay_config.h`): a green glob, a splat on walls. The spit clip releases at frame 7 of 18, the mouth at 0.87
  of the reference height.
* Journal moves `Rear` and `Spit`.
* Model `tools/blender/models/cobra.py`, sounds `tools/audio/cobra_sounds.py`. Check: `tests/scenarios/cobra.txt`.

## Next

* Play test: the look in game, the spit's timing, whether a jump really dodges it.
* Placement in the campaign (with antidotes in reach: the checker wants them).
