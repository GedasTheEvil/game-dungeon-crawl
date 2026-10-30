# Difficulty setting

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Opus: balance work across monsters, traps and loot.

## Idea

There is no difficulty option (`Settings`, the Options rows). The campaign has one balance
([monster-balance](solved/monster-balance.md), `tools/balance`). Offer three levels, chosen at New Game and stored in the
save and the run record:

* Easy: monster damage x0.6, monster health x0.8, more healing in chests.
* Normal: today's balance.
* Hard: monster damage x1.4, fewer potions, traps a little faster.

## Open points (implementer's choice, report them)

* Which knobs: damage only is simplest; health and loot change the balance sim. Keep the sim on Normal.
* Can it change mid-run? Suggest: no, it is part of the run; the Hall of Records shows it.
* Does Easy also lower the riddle difficulty (an extra hint)? Suggest: no.
