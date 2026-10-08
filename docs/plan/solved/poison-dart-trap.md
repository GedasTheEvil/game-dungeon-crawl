# Pressure plate with poison arrows

Status: solved, play-tested by the user 2026-10-08. Was: done 2026-10-08, to be play-tested. Drafted 2026-10-07, from the user.

## Idea

A new trap: a pressure plate in the floor. A player who steps on it sets off poison arrows that fly out of the wall
at them.

* The arrows poison: medium tier (`PoisonTier::Medium`, [poison-and-antidote](poison-and-antidote.md)).
* Jumping over the plate sets off nothing.
* Running (sprinting) across it does not help: the arrows still hit and poison.

## Weight (new mechanic)

Monsters get a weight, and the plate goes off only under a heavy one.

* Small monsters do not set it off: rats, scarabs, bats (bats fly anyway).
* Large monsters do, and the arrows hit them. Cowards (`Courage::Coward`) do not step on traps, so in practice the
  reckless ones: mummies, the Anubis guard, the bosses.
* Weight is a tier, a small whole number, in the monster row (`MonsterKind::weight`,
  `src/world/monster_kinds.cpp`):
  * 0: flyers (bats), they never touch the floor;
  * 1: small monsters (rats, scarabs);
  * 2-3: large monsters;
  * above 3 is fine and expected (bosses, later traps that need more weight).
* The plate goes off at weight 2 and above. The player sets it off too, so the player counts as at least 2.
* Other traps later may use the same tiers with their own threshold.
* Does the plate count as a trap for the cowards' fear (they stop at its edge), or is it hidden from them?
* Poison on a monster: [monster-poison](../monster-poison.md) (done): `Monster::TakePoison(PoisonTier::Medium, false, rng)`, no XP for its kill.
* The player's luring a mummy over the plate: a feature to keep.

## Decided by the implementer (the user was away)

* Where the darts come from (changed 2026-10-08 after the user's play test of lvl16; first built from the nearer side
  wall on the row, which was often off screen): three holes in a sandstone panel in the back wall right above the
  plate (`mechanism.py` `dart_holes`, drawn with the plate), always visible, so a careful player sees the trap coming.
  The darts fly out of them towards the camera, across the corridor. Works anywhere: every cell has a back wall.
* Pressing: a centre over the slab (within `DART_PLATE_HALF_WIDTH` 0.27 of the cell's middle), not anywhere in the
  cell.
* The volley: a click, then 3 darts 0.2 s later, 0.16 s apart, one per hole (left, middle, right: `DART_HOLE_X`), at 6
  tiles a second. At the walking line (half the corridor's depth) each takes the body in front of its hole (within
  `DART_HALF_WIDTH` 0.1): 2 pierce damage (armour helps) and medium poison. Missed, it flies on out of the corridor.
  Walking on does not get the player clear in time.
* Dodging: the holes and darts are 0.3 tiles over the floor, so a jump at its top lets one pass, and they fly over the
  small monsters (rats, scarabs). A running jump over the plate does not press it.
* Re-arms 3 s after the click (`DART_REARM_MS`), once nothing heavy stands on it: a mummy standing on it fires one
  volley, not one a tick. A save keeps no pressed plate (it loads armed).
* Weights (`MonsterKind::weight`, default 2): flyers 0; rat, scarab, scorpion, cobra 1; Anubis guard, crocodile 3;
  bosses 4, Sobek 5; the rest 2. The player is 2 (`PLAYER_WEIGHT`), the plate goes off at 2 (`DART_PLATE_WEIGHT`).
* Cowards treat the plate as a trap: they stop at its edge, a leaper leaps it (`Dungeon::walkerBlocked`).
* Monsters: the darts' damage is cut by `trapDamagePct` (a trap's), the poison rolled against their resistance
  ([monster-poison](../monster-poison.md)), no XP for the kill.
* Level format: tile type 14 `DartPlate`, value 0 armed / 1 pressed; glyph `_` in the ASCII sources.
* Checker: a plate costs 8 on the path (it jumps one when it can); a plate on the path wants an antidote in reach,
  like poisoners; the difficulty counts it (2 per plate on the path).
* Look and sound: a sandstone slab with a cobra carved on it, a dark gap round it, it sinks while pressed
  (`mechanism.py` `pressure_plate`); the hole panel above it (`dart_holes`); the darts are the bow's arrow, smaller; `plate_click.wav` and `dart.wav`
  (`tools/audio/mechanism_sounds.py`). The draft map sketches a slab and the three holes. The editor icon `dartplate.png`.
* Journal: the field note "Dart traps" on the first plate the player presses.
* Campaign: two to start with: lvl16 (the upper loop hall, a mummy east of it) and lvl23 (the blue lever's hall, an
  Anubis coming over it). Both have antidotes in reach; `./levelcheck` clean, `make paths` passes.

## Done

* `DartPlate` (`level.h`, `tile_defs.cpp`), `Dungeon::updateDartTraps`, `pressPlate`, `plateLoaded`, `updateDarts`
  (`src/world/dungeon_darts.cpp`), drawing in `dungeon_render_mechanisms.cpp` / `dungeon_render_effects.cpp`,
  `MonsterKind::weight`, the dart constants in `gameplay_config.h`, `level_check.cpp` (`pathDartPlates`), `FieldNote::DartTraps`.
* Tests: `tests/scenarios/dart_trap.txt` (`tests/levels/dart_trap.txt`), `tests/unit/walker_test.cpp`.

## Left for later

* Generated levels (`levelgen`) place none yet: it would need an antidote placed with it.
* Other weight traps (a collapsing floor for the heaviest) can reuse `MonsterKind::weight`.

## To test

* Whether the darts are visible enough in flight (small and fast), and the plate on the floor (meant to be subtle,
  readable once known).
* The jump dodge timing: fair, or too hard to ever use?
* The two campaign placements.
