## Process
After writing the code, run `make check`: it builds, fixes links to moved plans, formats, checks the format, runs tidy on
the changed files and the unit tests. Never format by hand: `make format` does it. The tree has no tidy warnings, so every warning
`make tidy` prints is yours: fix it (or `NOLINTNEXTLINE(check): reason` for a false positive).
Where is X: [docs/development.md](docs/development.md#where-is-x).

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

## Blender
Always use Blender 5: `/home/gediminas.skucas/Apps/blender-5.2.2-linux-x64/blender`.
Never use the system `blender` (v4.0).
The `blender` MCP server needs Blender 5 running with the MCP add-on started (port 9876).

## Modelling
Models are built procedurally in Blender Python scripts. Making one (steps, tools, format conventions):
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
(model tags, draft format, play-test commands, moving to "solved/", its tests).
The directory "docs/plan/" is used to offload ideas (in *.md files) for a latter use.
For example, when the user says let's leave this idea for later, save info about it as "<idea-slug>.draft.<model>.md" inside the plan directory.
Lifecycle: `<slug>.draft.<model>.md` → implemented `<slug>.md` → user-confirmed `solved/<slug>.md`.
A draft the user drops goes to `wont-do/<slug>.md` instead (docs/plans.md, Won't do).
Never rename or move a plan by hand: `tools/plan.py move <slug> implemented|solved|wont-do|draft-opus|draft-sonet` renames it
and rewrites every link to it. "Draft status", "what next": start from `tools/plan.py status` (ready, blocked, on
hold, waiting for a play-test), then read only the drafts you weigh.
The model tag `sonet` or `opus` says who can implement it. The default model stays Opus 5.5. When asked to implement
a `sonet` draft, don't do it yourself and don't ask: delegate it to a Sonnet 5.5 agent (Agent tool,
`model: sonnet`), then report its result. Tell the agent to follow AGENTS.md and docs/plans.md, run the checks,
go through the draft point by point before it renames the plan (and report any point it left out), and commit with a
Sonnet 5.5 `Co-Authored-By` line. `opus` drafts you implement yourself.
Code, tests and level files name a plan by its slug only (`// Climbers (plan monster-climbers): ...`), never by its
path or file name: a draft's rename or move to "solved/" then touches no code, so solving one needs no rebuild and
no test run. Only docs (markdown) link plans by path. Old path references in code: shorten them when you touch them.
New draft plans can arrive (and get committed) at any time, also in parallel with other work. They are not blocking:
ignore them, don't stop or change the current task because of them, and don't include them in your own commits.

## Git
In this project, commit your changes yourself when a task is done, without asking.
Trunk-based: `master` is the only long-lived branch and the only one that gets pushed.
Parallel tasks may run in a git worktree on a short-lived task branch, without asking. When the task is done:
1. Rebase the task branch onto `master` (no need to ask), fix conflicts, rerun `make`, the style checks and the
   tests there.
2. Merge it into `master` in this main folder, not in the worktree: the user play-tests here. If the merge would
   touch files with uncommitted changes in the main folder, stop and ask.
3. Rebuild here (`make`), then remove the worktree and delete the task branch.

This overrides the global "no git actions" and "no worktrees" rules for these steps only.
Still ask before other git actions (push, rewriting `master` history, config, other branches).

## Old save games
This game has no real players yet. Old saves can mostly be ignored.

## Code style (Preferred, but optional)
When a method call is inline, format all args in new lines.
Bad:
```cpp
   glBindTexture(GL_TEXTURE_2D,
                           texture.texID); // a texture from now on (glIsTexture), freed or not before the upload
```
Good: 
```cpp
// a texture from now on (glIsTexture), freed or not before the upload
   glBindTexture(
      GL_TEXTURE_2D,
      texture.texID
   ); 
```
This code style is less prone to be reformatted when changes happen.
`make format` applies it to every call too long for one line (a call that fits stays on one line). A trailing comment
that makes the line too long: move it above the line by hand.
