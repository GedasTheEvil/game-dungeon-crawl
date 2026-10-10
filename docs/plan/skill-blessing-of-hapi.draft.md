# Skill: blessing of Hapi

Status: draft 2026-10-10. Needs [skills](skills.draft.md) first; the stacking part needs
[water-blessing-potion](water-blessing-potion.draft.md) (skip that part if the potion is not in yet, add it with the
potion). From the user.

## Idea

A permanent, weaker form of the water blessing potion, and a boost when both are on.

| | Wade speed | Sprint in half water | Jump out of half water |
|---|---|---|---|
| Today | 50% | no | no |
| Skill | 75% | no | yes |
| Potion (the potion draft) | 100% | yes | yes |
| Skill + potion | 125%, faster than on land (a swimmer, like the crocodile) | yes, stamina drain halved | yes |

* The potion lasts twice as long with the skill (4 minutes): the god knows his own.
* Naming: the potion is "Blessing of Hapi" too. Rename one: the skill **"Favour of Hapi"** (default), or the potion
  "Draught of Hapi" (implementer's choice; the card and the potion must not share a name).
* `levelcheck` keeps assuming neither (both are optional).
* Journal note, humour welcome: "The water parts for me now. Not much, but enough to stop wading like a heron."

## Tests

* Unit / sim: wade speed and jump with the skill; skill + potion gives 125% and a 4 minute potion.
