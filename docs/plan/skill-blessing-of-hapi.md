# Skill: blessing of Hapi

Status: draft 2026-10-10. Skill part implemented 2026-10-17; sprint in water with the skill added 2026-10-17, not
implemented yet. Then waiting for play-test:
`bin/game --play tests/play/skill-blessing-of-hapi.txt`. The stacking part (125%, full sprint, 4 minute potion) waits for
[water-blessing-potion](water-blessing-potion.draft.sonet.md), which must add it. From the user.

The skill is on the Heka screen already, sealed: implementing it clears its `sealed` in `SKILLS`
(`src/world/skills.cpp`) and checks its card text there.

## Idea

A permanent, weaker form of the water blessing potion, and a boost when both are on.

| | Wade speed | Sprint in half water | Jump out of half water |
|---|---|---|---|
| Today | 50% | no | no |
| Skill | 75% | yes, at 50% of the land sprint speed | yes |
| Potion (the potion draft) | 100% | yes, at 40% of the land sprint speed | yes |
| Skill + potion | 125%, faster than on land (a swimmer, like the crocodile) | yes, full land sprint speed, stamina drain halved | yes |

Sprint in half water (user 2026-10-17): the speed is the share of the land sprint (`WALK_SPEED` x3), not of the
wade speed. Stamina drains as on land, except skill + potion (halved). The skill row is part of the skill, so it
is in without the potion.

* The potion lasts twice as long with the skill (4 minutes): the god knows his own.
* Naming: the potion is "Blessing of Hapi"; the skill card is already "Favour of Hapi" (`SKILLS`). Keep both.
* `levelcheck` keeps assuming neither (both are optional).
* Journal note, humour welcome: "The water parts for me now. Not much, but enough to stop wading like a heron."

## Tests

* Unit / sim: wade speed, sprint (50% of land) and jump with the skill; skill + potion gives 125% and a 4 minute potion.
