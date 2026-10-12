# Anubis fire bolt

Status: draft 2026-10-12. Needs [fire-damage](fire-damage.draft.opus.md) first (the fire type and burning). From the
user: once fire exists, the Anubis's ranged attack is fire, and the bolt looks like fire (the torch flames).

## Today

* The Anubis bolt is the POC of [anubis-ranged-attack](anubis-ranged-attack.draft.opus.md): `SpitRules` with no
  poison, `colour` `BOLT_RED`; guard {50, 1.4 tiles, 3 s}, boss {125, 2.0 tiles, 2.5 s}
  (`src/world/monster_kinds.cpp`). Its damage uses the type's melee mix (`ANUBIS_MIX`, 80 blunt / 20 pierce):
  `Dungeon::Venom::from` carries the type, the hit takes `attackMix`.
* Look: `Dungeon::drawVenoms` (`src/world/dungeon_render_effects.cpp`), an emissive glob stretched along its flight,
  a flattening splat on a wall or the floor.
* Torch flames are not a shader: `Fire::draw` (`src/graphics/fire.h`), stateless camera-facing sprites, yellow ->
  orange -> red -> smoke, a `FireStyle` per source (`TORCH`, `BRAZIER`, `OIL_LAMP`). Torch light: `Lighting::add` with
  a `LightDef` (`Lighting::TORCH`), flicker per seed.

## Idea

* **Damage:** the bolt is 100% fire. Melee stays `ANUBIS_MIX`. So `SpitRules` gets its own optional mix
  (`std::optional<DamageMix> mix`; nullopt: the type's `attackMix`, as the cobras keep), and the venom hit uses it.
  Damage numbers stay (50 / 125); the fire hit sets the player burning (fire-damage's rule).
* **Boss:** same fire bolt, burning lasts longer (boss rule in `AGENTS.md`), e.g. 5 s vs the default 3 s.
* **Look:** a small fireball. Keep a smaller emissive core in an orange (`BOLT_FIRE` replaces `BOLT_RED`), and draw a
  `Fire::draw` with a new `FireStyle` (`Fire::BOLT`, about `OIL_LAMP` size, short life, little smoke) at the bolt's
  position every frame; the flames rise and lag behind it on their own. Seed per venom (its spawn time).
* **Light:** the bolt carries a moving light (`Lighting::add`, a `LightDef` like `OIL_LAMP`, flickering) so it lights
  the corridor as it flies. Check the frame: lights are added before `commit()`, the venoms are drawn after the opaque
  scene; the light needs adding in the lights pass.
* **Splat:** on a wall or the floor a short burst of fire (a `BRAZIER`-like style fading over `VENOM_SPLAT_MS`) instead
  of the flattening glob.
* **Journal:** the move reads "Hurls a bolt of fire from afar. A jump clears it." (today "a bolt of red light").
  The Anubis's line can mention its stone is tough to fire (fire-damage gives it fire tough).

## Open (for the implementer)

* Fire share: 100% fire is the default; a 70 fire / 30 blunt bolt is the fallback if fire warding makes it trivial.
  Check with the `monster-balance` sim and the Anubis `threat` (8, boss 15).
* Does the sceptre-thrust clip (still to do in anubis-ranged-attack) matter here? No; the release frame only moves the
  spawn point.
* Sound: a whoosh / crackle for the bolt if a fire sound exists by then, else the attack sound stays.
* The cobra spit becoming partly fire (fire-damage's default) uses the same `SpitRules::mix` and the same fire look;
  share the drawing (a glob is fire when its mix has fire).

## Tests

* Unit (`tests/unit/sim_test.cpp`): the bolt hit is fire and sets burning; the melee hit is not fire; boss burning
  lasts longer.
* Scenario with screenshots: the fire bolt in flight in a dark corridor (light on the walls) and its splat; extend
  `tests/scenarios/anubis_bolt.txt`.
