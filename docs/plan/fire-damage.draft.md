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

* Default: the giant cobra's spit becomes partly fire (the uraeus spits fire at the pharaoh's enemies), and Apep, its
  boss, more so and longer (boss rule: the boss whose `kin` is the cobra).
* Optional: the Bat of Shezmu / vampire bat, Sobek: no.

### Resistances

| Monster | Fire |
|---|---|
| Mummy | weak (200%): old linen and resin. The Anubis boss summons them; the boss itself is not weak (no boss has a weakness) |
| Man-eater plant / thorn acacia, egg cluster, worm | weak |
| Crocodile, Sobek, the water lotus (if it exists) | resists |
| Anubis living statue (stone) | tough |
| Bosses | normal or better |

* Player: a **Fire Warding** amulet (Egyptian name per [egyptian-names](egyptian-names.draft.md), e.g. "the feather
  of Ma'at" or "the sun disc"), four tiers like the other wardings.

## Also

* Journal: resistance rows show fire. Field note the first time something burns (humour welcome: "Mummies, it turns
  out, are mostly kindling.").
* Checker / sim balance: `monster-balance` sim runs with the new mixes.
* Glossary rows: fire, burning.

## Tests

* Unit: `resistedDamage` with fire; a mummy takes double from fire; burning ticks and stops in half water.
* Scenario with a screenshot: a burning mummy.
