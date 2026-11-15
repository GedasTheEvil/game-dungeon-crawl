# Test weapons: 1-damage pure blunt, stab, cut and fire weapons and a bow for scenarios

Status: solved 2026-11-15. Was: draft 2026-11-15, size M, needs nothing first.

## Problem

Scenarios that need several hits on one monster (hit reactions, stun, bleed and poison rolls, health bars, journal
"tried" notes, boss phases) kill it too soon with the real weapons. A weapon that hits with the real swing (reach,
motion, sounds, hit reaction) but takes exactly 1 HP per hit lets a script swing as often as it needs.

## Plan

* Four melee weapons, test-only, appended after the last melee id (`give melee 8|9|10|11`; don't shift the ids of the
  others, saves and scenarios use them):

  | label (`equip`) | name | mix (blunt, slash, pierce, fire) | model, motion, range, sounds |
  |---|---|---|---|
  | `smash_test` | Smash Test | 100, 0, 0, 0 | the club's |
  | `stab_test` | Stab Test | 0, 0, 100, 0 | the knife's (`dagger`) |
  | `cut_test` | Cut Test | 0, 100, 0, 0 | the sword's |

  | `fire_test` | Fire Test | 0, 0, 0, 100 | the club's (a fire hit, so it sets the monster burning as usual) |

  And one ranged test weapon, appended after the last ranged id (`give ranged 5`):

  | label | name | mix | model, motion, flight |
  |---|---|---|---|
  | `bow_test` | Bow Test | 0, 0, 100, 0 | the self-bow's; unlimited shots (`expect shots` stays at its max), no `minRange` |

  A tint or a name prefix is enough to tell them from the real ones in a screenshot; no new Blender model.
* Exactly 1 HP per hit, always: no item level growth, might, strength amulet, damage-type amulets (Crushing,
  Cleaving, Piercing), skills, power hit or a monster's weakness / resistance change it, and it never rounds to 0.
  One flag on `WeaponDef` (e.g. `testOnly`) drives this and the next point. The side effects of the type still roll
  as usual (bleed from a cut, stun from a blow), so they can be tested.
* Never in the game for a player: not in loot, chests, levelgen, shrine gifts, the level editor's item list, the
  level checker's weapon power, `bin/balance`, or the inventory's strength sort ranking. A level file naming one fails
  `levelcheck`. Only `give` and `equip` (scenarios, `tests/play/` scripts) reach them.
* The journal: a test weapon's hit counts as its damage type tried (that is a thing to test), the text shows the
  test name.
* Docs: `docs/testing.md` (`give melee` ids, `equip` labels), glossary row "test weapon".
* Unit test: each test weapon takes exactly 1 HP from a monster weak to its type, with might and a damage amulet on.
* A scenario that swings `smash_test` five times at a monster and expects its HP 5 lower.
