# Venom glob with green fire

Status: implemented 2026-10-12, play-tested and solved 2026-10-12. Look only:
the cobras' venom glob trails green, toxic flames. The damage does not change: the spit stays a poison attack, not fire (decided 2026-10-12).

## Today

* The cobra (`MonsterCobra`) and the giant cobra (`MonsterGiantCobra`) spit a venom glob (`SpitRules`, colour
  `VENOM_GREEN`, `src/world/monster_kinds.cpp`). Apep, their boss, has no spit; nothing to do for it here.
* `Dungeon::drawVenoms` (`src/world/dungeon_render_effects.cpp`) draws an emissive ball stretched along its flight and
  a flattening splat on a wall or the floor.
* `Fire::draw` (`src/graphics/fire.h/.cpp`): stateless flame sprites; the colours come from one hard-coded ramp
  (`RAMP` in `fire.cpp`: white-yellow, yellow, orange, red, embers, grey smoke).

## Idea

* **Palette per style:** `FireStyle` gets a pointer to its colour ramp (default: today's fire ramp, so torches,
  braziers and lamps stay as they are). Add a green ramp close to today's venom colour (`VENOM_GREEN` =
  {0.45, 0.75, 0.15}, `src/world/monster_kinds.h`): its core a lighter tint of that green, the body `VENOM_GREEN` itself, then darker
  green, then sickly grey-green smoke (decided 2026-10-12: stay close to the current venom colour).
* **Style:** `Fire::VENOM`: small (about `OIL_LAMP`), short life, a little more smoke than a flame, the vapour of the
  venom.
* **Glob:** no ball (play-test 2026-10-12: a ball under the flames read as a burning sphere, the user wants a fuzzy
  fire shape). `Fire::draw(Fire::VENOM, ...)` at its position, a cloud of flames, and two smaller `Fire::VENOM_TRAIL`
  clouds behind it along the flight. Seed per venom (its spawn time). The splat: a short green puff over
  `VENOM_SPLAT_MS`.
* **Only the venom:** a glob with poison gets the green fire. The Anubis bolt (no poison) keeps its own look, and
  [anubis-fire-bolt](../anubis-fire-bolt.draft.opus.md) later gives it the normal fire ramp the same way.
* **Light (decided 2026-10-12, do it):** a faint green moving light on the glob (`Lighting::add`, small radius, no
  flicker, colour `VENOM_GREEN`). Lights are added before `commit()`, the globs are drawn after the opaque scene; the
  light needs adding in the lights pass.

## Shared with the Anubis bolt (decided 2026-10-12)

One glob-with-flames drawing for both projectiles, the ramp (and style) picked per projectile: green for the venom,
the normal fire ramp for the Anubis bolt. Whichever of this and [anubis-fire-bolt](../anubis-fire-bolt.draft.opus.md)
comes second reuses the first one's drawing, with its own colours.

## Open (for the implementer)

* Exact ramp values and the style numbers: the implementer picks them, close to `VENOM_GREEN`. No screenshot tuning:
  the user checks the look in a play-test (decided 2026-10-12).

## Tests

* No screenshot scenario (decided 2026-10-12): the user verifies the look in a play-test.
* `make` and the existing tests pass; torches, braziers and lamps keep today's ramp (the default).
