## Process
After writing the code, run `make check`: it builds, fixes links to moved plans, formats, checks the format, runs tidy on
the changed files and the unit tests. Never format by hand: `make format` does it. Every warning `make tidy` prints is
yours (the tree has none): fix it. Code style, false positives: [docs/development.md](docs/development.md#build-and-run).
Where is X: [docs/development.md](docs/development.md#where-is-x).
Waiting for a long command, or a shell that seems to hang: [docs/stuck-processes.md](docs/stuck-processes.md). Never
wait with a `pgrep -f` loop.

## Glossary
Unsure of a term, or naming a new thing: grep [docs/glossary.md](docs/glossary.md) (one row per term, no need to read it all). Add new terms there.

## Glyphs
Level glyphs (monsters, tiles) are only read by tools and agents. Pick a free one yourself; never ask the user.

## Scenario tests
To check game behaviour or visuals, write a script in `tests/scenarios/` and run `make test SCENARIO=...`. Screenshots and results go to `tests/out/<name>/`. Reference: [docs/testing.md](docs/testing.md).
Any other run of `bin/game` by an agent (smoke checks, `--level`, `--play`) must be silent and bounded, or the user hears
its music: `xvfb-run -a timeout -k 5 N bin/game --no-sound ...`. Put `timeout` inside `xvfb-run` (outside, it kills only
the wrapper and the game lives on), with `-k` (the game ignores SIGTERM).

## UI screens
Menu, options, credits, inventory, riddle, map, journal and Heka share one look. Canvas, parts, colours, fonts and how to add a menu sub-screen: [docs/ui.md](docs/ui.md).

## Modelling
Models are built procedurally in Blender Python scripts, with Blender 5 only (never the system `blender`). Making one
(Blender path and MCP, steps, tools, format conventions):
[docs/modeling.md](docs/modeling.md). The finished remodel of the original models, notes per model:
[docs/remodeling.md](docs/remodeling.md).

## Command whitelist
These commands do not need confirmation to run
* make check
* make tidy
* make tidy-all
* make tidy-fix
* make format-check
* make unit
* make format
* make clean
* make

## Campaign levels
Every campaign level must pass the level checker with no warnings (`bin/levelcheck content/levels/lvl*`).
When a checker change flags a level, fix the level. Manual placement is no reason to keep a warning.
The level files (`content/levels/`) are the source of truth. They were generated from the seed table
(`tools/level/rebuild.txt`) and then polished by hand: never generate a level again unless asked, that drops what was
added since.

## Boss abilities
When a base monster gets a new ability (jump, ranged attack, swimming, ...), its boss gets one too (the boss whose
`kin` is that monster, `src/world/monster_kinds.cpp`). Make it stronger (farther, harder, faster) where that makes
sense. The same goes for draft plans: a plan that gives a base monster an ability covers its boss too.

## Plans
Writing, refining, reporting on, implementing or solving a plan: read [docs/plans.md](docs/plans.md) first
(lifecycle, model tags and who implements, draft format, plan names in code, play-test commands, moving to "solved/").
When the user leaves an idea for later, save it as `docs/plan/<idea-slug>.draft.<model>.md`.
Never rename or move a plan by hand: `tools/plan.py move <slug> ...`. "Draft status", "what next": start from
`tools/plan.py status` (ready, blocked, on hold, waiting for a play-test), then read only the drafts you weigh.
New draft plans can arrive (and get committed) at any time, also in parallel with other work. They are not blocking:
ignore them, don't stop or change the current task because of them, and don't include them in your own commits.

## Git
In this project, commit your changes yourself when a task is done, without asking. Parallel tasks may run in a git
worktree on a short-lived task branch, without asking. Branches, finishing a worktree task (rebase, merge, cleanup):
read [docs/version-control.md](docs/version-control.md) first.
Opus models push `master` to `origin` without asking, from the main folder only: after a commit there, or after merging
a worktree task into it. Never push from inside a worktree, never push task branches, never force-push. Other models
still ask before pushing.
This overrides the global "no git actions" and "no worktrees" rules for these steps only.
Still ask before other git actions (rewriting `master` history, config, other branches).

## Old save games
This game has no real players yet. Old saves can mostly be ignored.
