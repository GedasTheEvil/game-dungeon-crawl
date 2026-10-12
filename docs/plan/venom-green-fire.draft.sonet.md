# Venom glob with green fire

Status: draft 2026-10-12, ready (needs nothing first). Look only: the cobras' venom glob trails green, toxic flames.
The damage does not change: the spit stays a poison attack, not fire (decided 2026-10-12).

## Today

* The cobra (`MonsterCobra`) and the giant cobra (`MonsterGiantCobra`) spit a venom glob (`SpitRules`, colour
  `VENOM_GREEN`, `src/world/monster_kinds.cpp`). Apep, their boss, has no spit; nothing to do for it here.
* `Dungeon::drawVenoms` (`src/world/dungeon_render_effects.cpp`) draws an emissive ball stretched along its flight and
  a flattening splat on a wall or the floor.
* `Fire::draw` (`src/graphics/fire.h/.cpp`): stateless flame sprites; the colours come from one hard-coded ramp
  (`RAMP` in `fire.cpp`: white-yellow, yellow, orange, red, embers, grey smoke).

## Idea

* **Palette per style:** `FireStyle` gets a pointer to its colour ramp (default: today's fire ramp, so torches,
  braziers and lamps stay as they are). Add a green ramp: pale yellow-green -> green -> dark green -> sickly
  grey-green smoke.
* **Style:** `Fire::VENOM`: small (about `OIL_LAMP`), short life, a little more smoke than a flame, the vapour of the
  venom.
* **Glob:** keep the ball, a bit smaller, and draw `Fire::draw(Fire::VENOM, ...)` at its position each frame; the
  sprites rise and lag behind as it flies. Seed per venom (its spawn time). The splat: a short green puff over
  `VENOM_SPLAT_MS`.
* **Only the venom:** a glob with poison gets the green fire. The Anubis bolt (no poison) keeps its own look, and
  [anubis-fire-bolt](anubis-fire-bolt.draft.opus.md) later gives it the normal fire ramp the same way.
* Optional: a faint green moving light (`Lighting::add`, small radius, no flicker). Lights are added before
  `commit()`, the globs are drawn after the opaque scene; the light needs adding in the lights pass.

## Open (for the implementer)

* Colour values of the ramp and the style numbers: tune by screenshots.
* Whichever of this and anubis-fire-bolt comes second reuses the other's glob-with-flames drawing.

## Tests

* Scenario with screenshots: a cobra spits, the glob in flight and its splat (`tests/scenarios/`, like
  `anubis_bolt.txt`). Check a torch in the same shot still burns orange.
