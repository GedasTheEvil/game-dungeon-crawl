# Fire damage

Status: solved 2026-10-14 (play-tested by the user). From the user: a fire damage type, for the player and for monsters. Includes the
"mummies burn" idea from the lore review.

## Today

Three damage types (`DamageType`, `src/world/damage.h`): blunt, slash, pierce. A weapon's or monster's attack is a
`DamageMix` in percent; `Resistances` per type (weak 200%, normal, resists 50%, tough 25%); armour amulets per type.

## Idea

* A fourth type, **fire**. `DAMAGE_TYPE_COUNT` 4, `DAMAGE_TYPE_NAMES` "fire". Existing mixes get fire 0.
* A fire hit also sets the target **burning**: a short damage-over-time (default 3 s, a small share of the hit per
  second), shown with flames on the model (`src/graphics/fire.cpp`). Wading into half water puts it out. Burning is
  not poison: no antidote, no poison resistance.

### Player deals fire

Implementer's choice, at least one:
* **Oil flask**: a thrown item (like the javelin), bursts into fire on hit, short range.
* **Fire arrows**: dip arrows in a brazier (interact with a brazier: the next N bow shots carry fire).
* **Torch**: a short melee weapon, mostly fire.

### Monsters deal fire

* The cobras' spit stays poison, not fire (decided 2026-10-12).
* The first monster fire is the Anubis's bolt, after this plan: [anubis-fire-bolt](../anubis-fire-bolt.md).
* Optional: the Bat of Shezmu / vampire bat, Sobek: no.

### Resistances

| Monster | Fire |
|---|---|
| Mummy | weak (200%): old linen and resin. The Anubis boss summons them; the boss itself is not weak (no boss has a weakness) |
| Man-eater plant / thorn acacia, egg cluster, worm | weak |
| Crocodile, Sobek, the water lotus (if it exists) | resists |
| Anubis living statue (stone) | tough |
| Bosses | normal or better, and at least their kin's (`validKinds`): the Anubis boss tough, Sobek resists |

* Player: a **Fire Warding** amulet (Egyptian name per [egyptian-names](egyptian-names.md), e.g. "the feather
  of Ma'at" or "the sun disc"), four tiers like the other wardings: fire damage -20 / 40 / 60 / 80%. Never more
  than 80% (decided 2026-10-12): fire always hurts a little.
* **Fire ignores armour** (decided 2026-10-12): only the Fire Warding amulet (and a fire resistance) cuts it. Today
  `playerHitDamage` takes the armour's flat amount off the whole hit; split it: the fire share after resistances goes
  through untouched, the armour comes off the other shares only, and never below 0 for them (a mixed hit like the
  Anubis bolt, [anubis-fire-bolt](../anubis-fire-bolt.md): armour can soak its blunt / pierce part, never
  its fire). Burning ticks ignore armour too. The journal / amulet text can say so ("Armour does not stop fire").
  Unit test: a 100% fire hit on an armoured player deals the same as on an unarmoured one.
  Write the split as "armour acts on the non-fire shares", whatever armour does: the
  [monster-balance](../monster-balance.draft.opus.md) sim may turn the flat cut into a percentage.

## Also

* Journal: resistance rows show fire. Field note the first time something burns (humour welcome: "Mummies, it turns
  out, are mostly kindling.").
* Checker / sim balance: `monster-balance` sim runs with the new mixes.
* Glossary rows: fire, burning.

## Tests

* Unit: `resistedDamage` with fire; a mummy takes double from fire; burning ticks and stops in half water.
* Scenario with a screenshot: a burning mummy.

## Implemented

* `DamageType::Fire` (`src/world/damage.h`), `DAMAGE_TYPE_COUNT` 4; every mix and resistance row has its fire column
  (a zero rate is refused by `validKinds`: every type hurts). `playerHitDamage` takes the armour off the non-fire
  shares only (`fireDamage` is the fire share after the resistances).
* Burning: `Burning` in `damage.h`, `BURN_MS` 3 s at `BURN_TICK_PERCENT` 20% of the hit's fire share a second
  (hundredths carried); a stronger fire takes over, a weaker one restarts the time. `Monster::UpdateBurning`,
  `Player::UpdateBurning` from `Dungeon::updateBurning`; half water douses (the monster's `InWater`, the player's
  `PlayerSink`). A monster burnt to death is the player's kill (only their weapons set fire). Not saved (seconds).
* Player fire: the **fire arrows**. Interact in a brazier's cell with a bow owned (`Dungeon::lightArrows`): the next
  `FIRE_ARROW_SHOTS` 5 arrows carry fire, `FIRE_ARROW_PERCENT` 60% of the damage (40 pierce / 60 fire, so the journal
  learns fire from them). Every campaign level with mummies has at least one brazier (counted at implementation).
  Not saved. Look: `Fire::ARROW` on the tip, a light (`addFireLights`).
* Look of burning: `Fire::body` (`Fire::BODY` flames up the box), on monsters (`Dungeon::drawBurning`) and the player
  (`draw.cpp`), with a flickering light.
* Resistances as the table; also worm and egg cluster weak, the mimic and the rest normal. The Anubis boss stays
  tough, Sobek resists, through `bossResist`.
* Fire Warding: `AmuletType::FireWard`, the **Sun Disc** (Ra's disc in carnelian between gold cow horns,
  `build_amulet_fire` in `tools/blender/models/items.py`, icon `sun_disc` in `tools/textures/hud_icons.py`), ids
  38-41, 20 / 40 / 60 / 80%; lore line "Armour does not stop fire."
* Journal: the FIRE field in the TRIED form (POISON moved under it; journal version 8 moves the old poison bit),
  `attackSentence` knows fire, the field note "Fire" the first time something burns, status lines "Mummy catches
  fire", "You are on fire!", "The water puts the fire out".
* Scenario command `firehit N` (a fire hit on the player), `expect burning` / `nearest_burning`.
* Tests: `tests/unit/damage_test.cpp` (fire share, armour split, Burning), `tests/unit/monster_rules_test.cpp` (fire
  arrows on the mummy, the player burning and the pool), `tests/scenarios/solved/fire_mummy.txt` (parked) (screenshots: burning
  mummy, burning player, the journal's fire row), test level `tests/levels/fire`.
