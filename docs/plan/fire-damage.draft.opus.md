# Fire damage

Status: draft 2026-10-10, ready (needs nothing first). From the user: a fire damage type, for the player and for
monsters. Includes the "mummies burn" idea from the lore review.

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
* The first monster fire is the Anubis's bolt, after this plan: [anubis-fire-bolt](anubis-fire-bolt.draft.opus.md).
* Optional: the Bat of Shezmu / vampire bat, Sobek: no.

### Resistances

| Monster | Fire |
|---|---|
| Mummy | weak (200%): old linen and resin. The Anubis boss summons them; the boss itself is not weak (no boss has a weakness) |
| Man-eater plant / thorn acacia, egg cluster, worm | weak |
| Crocodile, Sobek, the water lotus (if it exists) | resists |
| Anubis living statue (stone) | tough |
| Bosses | normal or better |

* Player: a **Fire Warding** amulet (Egyptian name per [egyptian-names](solved/egyptian-names.md), e.g. "the feather
  of Ma'at" or "the sun disc"), four tiers like the other wardings: fire damage -20 / 40 / 60 / 80%. Never more
  than 80% (decided 2026-10-12): fire always hurts a little.
* **Fire ignores armour** (decided 2026-10-12): only the Fire Warding amulet (and a fire resistance) cuts it. Today
  `playerHitDamage` takes the armour's flat amount off the whole hit; split it: the fire share after resistances goes
  through untouched, the armour comes off the other shares only, and never below 0 for them (a mixed hit like the
  Anubis bolt, [anubis-fire-bolt](anubis-fire-bolt.draft.opus.md): armour can soak its blunt / pierce part, never
  its fire). Burning ticks ignore armour too. The journal / amulet text can say so ("Armour does not stop fire").
  Unit test: a 100% fire hit on an armoured player deals the same as on an unarmoured one.

## Also

* Journal: resistance rows show fire. Field note the first time something burns (humour welcome: "Mummies, it turns
  out, are mostly kindling.").
* Checker / sim balance: `monster-balance` sim runs with the new mixes.
* Glossary rows: fire, burning.

## Tests

* Unit: `resistedDamage` with fire; a mummy takes double from fire; burning ticks and stops in half water.
* Scenario with a screenshot: a burning mummy.
