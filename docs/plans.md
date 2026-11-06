# Plans

The plan lifecycle in `docs/plan/`: draft, implemented, solved (or won't do). The rules every task needs (save an
idea as a draft, never rename by hand, drafts that arrive mid-task) are in [AGENTS.md](../AGENTS.md#plans).
`tools/plan.py` does every step below; run it without arguments for its commands. `make check` runs `plan.py links`
and `plan.py lint` (names against Status lines, sizes) and the tool's tests (`tools/plan_test.py`).

## Lifecycle

`<slug>.<kind>.draft.<model>.md` → implemented `<slug>.<kind>.md` → user-confirmed `solved/<slug>.<kind>.md`. A draft
the user drops goes to `wont-do/<slug>.<kind>.md` instead ([Won't do](#wont-do)). Old plans have no kind (features).
`tools/plan.py move <slug> implemented|solved|wont-do|draft-opus|draft-sonet|kind-bug|kind-feature|kind-tooling`
renames a plan, rewrites every link to it and in it, and its Status line. Links inside solved and won't-do plans are
not checked (`tools/plan.py links`, `make check`): they are history.

| Ask | Command |
|---|---|
| "draft status" | `tools/plan.py status` (rework, play-test, drafts by kind: ready, blocked, on hold) |
| "what next", "something small" | `tools/plan.py next [--size S\|M\|L] [--kind bug] [--model sonet]` |
| "what do I play-test", "show me the test" | `tools/plan.py playtest [SLUG]`: the command and the checklist |
| the user's play-test reply | `tools/plan.py verdict "ok a" "no b: too slow"` |
| "is there a plan about X", an old plan by topic | `tools/plan.py find <term>...` (drafts, implemented, solved, won't do; all terms, case-insensitive) |
| a new idea | `tools/plan.py new <slug> --kind K --model M --size S [--prio P] Title words` |
| "hold X", "X needs Y first" | `tools/plan.py hold X reason`, `unhold X`, `needs X Y`, `unneed X Y` |
| an overview for the user | `tools/plan.py board` writes `docs/plan/BOARD.md` (not tracked) |

## Plans in code

Code, tests and level files name a plan by its slug only (`// Climbers (plan monster-climbers): ...`), never by its
path or file name: a draft's rename or move to "solved/" then touches no code, so solving one needs no rebuild and
no test run. Only docs (markdown) link plans by path. Old path references in code: shorten them when you touch them.

## Kind

The `<kind>` in the file name says what the plan is:

* `bug`: the game does something wrong (a crash, a wrong rule, a wrong text, the checker passing what the game
  blocks). `status` lists bugs first, `next` picks them first. A bug fix comes with a test that names the plan and
  failed before the fix (`done-check` fails without one). A bug draft says Repro, Expected, Actual, Fixed when.
* `feature`: something new or changed on purpose.
* `tooling`: build, tools, checks, code style, docs; no gameplay, visuals, levels or balance. It needs no play-test:
  `move <slug> implemented` moves it straight to `solved/` ([Solved](#solved)).

Unsure: a fix of what a plan or the docs promised is a bug; anything they did not promise is a feature.
Change it with `tools/plan.py move <slug> kind-bug` (or `kind-feature`, `kind-tooling`).

## Model tag

The `<model>` in `<slug>.<kind>.draft.<model>.md` says who can implement it:

* `sonet`: simple, delegate to a Sonnet agent. Narrow and fully specified, follows an existing pattern (a potion,
  an amulet, a rename, a skill rule), no new Blender model or clip, no level design, no balance judgement.
* `opus`: complex, needs Opus. A new system or monster, new models or animation clips, level design, checker
  modelling, balance work, or several systems at once.

When refining a draft changes its size, retag it: `tools/plan.py move <slug> draft-opus` (or `draft-sonet`).

The default model stays Opus 5.5; `opus` drafts you implement yourself. When asked to implement a `sonet` draft,
don't do it yourself and don't ask: delegate it to a Sonnet 5.5 agent (Agent tool, `model: sonnet`), then report its
result. Tell the agent to follow AGENTS.md and docs/plans.md, run the checks, go through the draft point by point
before it renames the plan (and report any point it left out), pass `tools/plan.py done-check <slug>`, and commit with
a Sonnet 5.5 `Co-Authored-By` line. A worktree for it starts from the local `master`, not `origin/master`
([version-control.md](version-control.md#worktrees)). Run `done-check` yourself before you merge its work.

## Writing a draft

Start one with `tools/plan.py new` (the skeleton for its kind, the Status line with date and size). Drafts are
written for agents to implement later. Every draft is ready to implement as is; only a link to another draft it needs
first blocks it. Open points or several ideas in a draft are the implementer's choice.
Still, when reporting on drafts, warn about their open questions; when asked to refine a draft, list them.

Optimize for agent reading: the first lines (status) say what it needs first, if anything, so readiness is clear
without reading the whole file. `tools/plan.py status` reads them:

* `Status: draft <date>, size S|M|L[, prio high|low], needs nothing first.` The first date is its age. The size is
  required (`lint`): S an hour or less, M a few hours, L a day or more. No prio: normal.
* A sentence with "needs" and a link to an unsolved plan blocks it ("Needs first:" and the link; "needs nothing
  first" does not). Set it with `plan.py needs <slug> <other>` (`unneed` drops it).
* "On hold" holds it: `plan.py hold <slug> <reason>`, `unhold <slug>`.

After a grill or a refine with the user, write every decision into the draft before the task ends, and resize it.

A plan that gives a base monster an ability covers its boss too (AGENTS.md, Boss abilities).

## Implemented

Once a draft plan is implemented: `tools/plan.py move <slug> implemented`. It runs `done-check` first and refuses on a
FAIL line (`--force` moves anyway; say why in the commit):

* a bug: a unit test or scenario that names the plan;
* a bug or a feature: a play command (`tests/play/<slug>.txt`, or `bin/game --level N` in the plan) and a
  `## Play-test` section: 2 to 5 lines, what the user does and what they should see;
* warnings: no code or test names the plan, a play script whose level changed after it;
* the reminders it prints (draft points, journal and glossary rows, boss abilities, Where is X).

The move drops the `draft.<model>` part, rewrites the links and starts the Status line with "implemented <today>,
waiting for the user's play-test. Was: ...".

The play command starts right at the feature, so the user doesn't play through the campaign: `bin/game --level N`
(a campaign level that has it), or a script in `tests/play/` (setup commands: `god`, `level`, `give`, `curse`, ...;
committed, `make test` skips it) for `bin/game --play tests/play/<plan-slug>.txt`.

## Play-test

When the user asks what to play-test, give them `tools/plan.py playtest` (or `playtest <slug>`): per plan the command,
its checklist, the days it has waited, and warnings (no command, no checklist, a play script whose level changed).

The user replies per plan, "ok <slug>" or "no <slug>: what is wrong" (also ✓ / ✗, one per line or `;`). Feed the reply
to `tools/plan.py verdict ...`: an ok solves the plan (play-tested), a no adds a round to its `## Play-test feedback`
section with one `- [ ]` item per note. `reject <slug> <note>...` does the same for one plan. `status` and `next`
show it as rework, first in line. Fix the items, tick them `- [x]` and give the user the play command again; the
plan stays implemented until an ok.

## Solved

Once the user confirms an implemented plan is solved (tested / verified): `tools/plan.py solve <slug>...
--playtested` (several at once is fine; `verdict` calls it). It:

* moves the plan to `docs/plan/solved/`, rewrites the links and starts its Status line with
  "solved <today> (play-tested by the user). Was: ...";
* parks the scenarios that are about it (their first comment line names the plan) in `tests/scenarios/solved/`
  and rewrites the paths to them; a scenario that only mentions it is listed, park it by hand if it is about it;
* removes its play script (`tests/play/` holds only plans waiting for a play-test);
* prints the commit message.

Exception: a plan that does not change the game (build, tooling, code style, docs, checks; no gameplay, visuals,
levels or balance) needs no user confirmation: its kind is `tooling`, and `move <slug> implemented` solves it.

When a plan moves to `docs/plan/solved/`, turn the behaviour checks of its scenarios (`expect` lines; `solve` lists
the parked ones that have them) into unit tests (`tests/unit/`, [testing.md](testing.md)). A scenario that needs a
screenshot stays in `tests/scenarios/solved/`: `make test` skips it, `make test SCENARIO=...` still runs it; kept for
history, it may fail once the rules change (no fix needed). Goal: lean, fast tests.

## Won't do

A draft the user drops (no longer needed, superseded, rejected) is not deleted:
`tools/plan.py move <slug> wont-do` moves it to `docs/plan/wont-do/` and rewrites the links. First add the reason and
the date to its status line ("Won't do 2026-10-27: ..."; `lint` fails without it). `status` counts it and no longer
lists it; a draft that needs it shows "needs <slug> (won't do)" and fails `lint`: rethink that draft. Only on the
user's word, never on an agent's own judgement.

## Priority

Being newer does not make a draft-plan more important than the older ones. `next` orders: rework, then bugs, then
`prio high`, normal, `low`, then the oldest. A `prio` is the user's word, never an agent's own judgement.
