# Skill: dodge

Status: draft 2026-10-10. Needs [skills](skills.draft.md) first. From the user; not fully refined.

## Idea

A new mechanic: with the skill, the player has a 10% chance to avoid a monster's attack entirely.

* Rolls on each monster hit on the player (melee bites, stings, spit, bolts): no damage, no poison. Traps, rock falls
  and darts: not dodged (default; implementer's choice).
* Feedback: a quick sidestep of the view (a small camera nudge), a whoosh sound, a "Dodged" status line or floating
  word.
* Rolls on the gameplay stream.
* Later ideas (not in this draft): a dodge amulet, dodge raised by agility.

## Open (warn when reporting)

* Whether dodge works while wading, on a ladder or mid-jump. Default: everywhere.

## Tests

* Unit / sim: about 10% of hits dodged (seeded), a dodged poisoner hit does not poison.
