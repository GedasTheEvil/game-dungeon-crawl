# Plans

The plan lifecycle in `docs/plan/`: draft, implemented, solved. The core rules (naming, who implements, slug-only
names in code, drafts that arrive mid-task) are in [AGENTS.md](../AGENTS.md#plans).

## Model tag

The `<model>` in `<idea-slug>.draft.<model>.md` says who can implement it:

* `sonet`: simple, delegate to a Sonnet agent. Narrow and fully specified, follows an existing pattern (a potion,
  an amulet, a rename, a skill rule), no new Blender model or clip, no level design, no balance judgement.
* `opus`: complex, needs Opus. A new system or monster, new models or animation clips, level design, checker
  modelling, balance work, or several systems at once.

When refining a draft changes its size, retag it: `tools/plan.py move <slug> draft-opus` (or `draft-sonet`).

## Writing a draft

Drafts are written for agents to implement later. Every draft is ready to implement as is; only a link to another
draft it needs first blocks it. Open points or several ideas in a draft are the implementer's choice.
Still, when reporting on drafts, warn about their open questions; when asked to refine a draft, list them.

Optimize for agent reading: the first lines (status) say what it needs first, if anything, so readiness is clear
without reading the whole file. `tools/plan.py status` reads them: a sentence with "needs" and a link to an unsolved
plan blocks the draft ("Needs first:" and the link; "needs nothing first" does not), "On hold" holds it, the
first date is its age.

A plan that gives a base monster an ability covers its boss too (AGENTS.md, Boss abilities).

## Implemented

Once a draft plan is implemented: `tools/plan.py move <slug> implemented` (drops the `draft.<model>` part, rewrites
the links to it).

When an implemented plan waits for the user's play-test, give the user a command that starts right at the feature,
so they don't play through the campaign: `bin/game --level N` (a campaign level that has it), or a script in
`tests/play/` (setup commands: `god`, `level`, `give`, `curse`, ...; committed, `make test` skips it) for
`bin/game --play tests/play/<plan-slug>.txt`. Not needed when the plan changes the campaign itself.

## Solved

Once the user confirms an implemented plan is solved (tested / verified): `tools/plan.py move <slug> solved`. It moves
the plan to `docs/plan/solved/`, rewrites the links and lists the scenarios and play scripts that name it.

Exception: a plan that does not change the game (build, tooling, code style, docs, checks; no gameplay, visuals,
levels or balance) needs no user confirmation. Once implemented and its checks pass, move it straight to
`docs/plan/solved/` (`tools/plan.py move <slug> solved`) in the same commit.

When a plan moves to `docs/plan/solved/`, turn the scenario tests it uses into unit tests (`tests/unit/`,
[testing.md](testing.md)). A scenario that needs a screenshot moves to `tests/scenarios/solved/`: `make test` skips
it, `make test SCENARIO=...` still runs it as a slow regression check. Goal: lean, fast tests.

## Priority
Being newer does not make a draft-plan more important than the older ones.
