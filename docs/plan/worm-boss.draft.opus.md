# Worm boss: the serpent of the sands

Status: draft 2026-10-20, from the user ("a boss of the worm type for the new desert section"). Needs first:
[desert-chapter](solved/desert-chapter.md) (the sand it burrows in, the worm's sand burrowing).

## Lore

Egyptian myth has no giant worms. It has many serpents, and some of them live in sand:

* The fourth and fifth hours of the Amduat: the land of Sokar (Rosetau) is a desert of sand. The sun's barque turns
  into a serpent and is dragged through it, and serpents with legs, wings or three heads crawl over it.
* Spells in the Pyramid Texts and the Book of the Dead ward off biting snakes and "the worm" (`rerek` is one of the
  names) that comes out of the earth.

So the boss is a sand serpent of Sokar's land: long, eyeless, ringed like the worm, with a snake's head. The worm
stays the sand worm of today (the Shai-Hulud joke in its note stays). Apep is already the cobra's boss (a serpent of
the waters of chaos), so this one has to look different from him: blind, a burrower's mouth, no hood. Implementer's
choice of name, for example "Rerek" or "the Serpent of Rosetau" (check the name against the sources before using it).

## Idea

`MonsterRerek` (working name), kin `MonsterWorm`. Boss rule (AGENTS.md): the worm's sand burrowing, stronger.

* Model: the worm's, scaled up (about 2.5x), own texture (sand-coloured rings, a dark head with serpent scales), or
  its own model with a snake's head (more work). Clips as the worm's, plus a dive and a rise if the worm has none.
* **Burrow in sand, stronger:** dives from farther, comes up under the player or on the far side of them (Apep's
  `BURROW_BEHIND`), and moves under the sand faster. Out of sand it walks, slowly.
* **The extra ability, one of (implementer's choice):**
  * **Poison**: its bite poisons (medium). It would be the worm family's first poison. The user's idea; fits a serpent.
  * **Swallow**: a hold like the crocodile's (`HoldRules`), the player pulled into the sand; the hits on it break the
    hold sooner (the scorpion's `breakMs`).
  * **Sand spray**: as it comes up, sand sprays out round it (the storm roar's sand haze, `Dungeon::SandHaze`).
* Minions: sand worms dig out (`Summon::DigOut`), in sand, where they lurk under the surface.
* Weakness: worms resist blunt and are weak to slash and fire; `bossResist` drops the weaknesses.
* Journal note in the archaeologist's tone.

## Placement (open)

The desert chapter has one boss slot (Seth, lvl25). Options:

* A mid-chapter boss (lvl23) in a sand pit: the campaign's first boss off the every-fifth-level rhythm.
* Seth moves on and this boss takes lvl25. Seth then needs another slot: Sobek's halls are also water.
* No campaign slot, play script only, as hippo-boss did at first.

## Bossless monsters (for reference)

Without a boss of their kin today: rat and giant rat, plant, mimic and cursed mimic, lotus, camel spider, worm (this
plan), and the egg cluster (a nest, not a monster to fight). The ushabti steps out of the wall with the Anubis boss,
but it is not his kin.

## Open (warn when reporting)

* The name, and whether it keeps the worm model or gets a serpent's head.
* The extra ability: poison, swallow or sand spray.
* The placement.

## Tests

* Unit / sim: it lies under the sand, comes up under or past the player, dives from farther than the worm, the extra
  ability works, worms dig out in sand.
* Scenario with a screenshot: under the sand (the ripple), coming up, the extra ability.
