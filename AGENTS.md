## Process
After writing the code, verify it compiles by running `make`.
Once in compiles, run the code style checks `make format`, `make format-check` and `make tidy`.

## Glossary
Unsure of a term, or naming a new thing: grep [docs/glossary.md](docs/glossary.md) (one row per term, no need to read it all). Add new terms there.

## Glyphs
Level glyphs (monsters, tiles) are only read by tools and agents. Pick a free one yourself; never ask the user.

## Scenario tests
To check game behaviour or visuals, write a script in `tests/scenarios/` and run `make test SCENARIO=...`. Screenshots and results go to `tests/out/<name>/`. Reference: [docs/testing.md](docs/testing.md).
Any other run of `./game` by an agent (smoke checks, `--level`, `--play`) must be silent and bounded, or the user hears
its music: `xvfb-run -a timeout -k 5 N ./game --no-sound ...`. Put `timeout` inside `xvfb-run` (outside, it kills only
the wrapper and the game lives on), with `-k` (the game ignores SIGTERM).

## UI screens
Menu, options, credits, inventory, riddle and map share one look. Canvas, parts, colours, fonts and how to add a menu sub-screen: [docs/ui.md](docs/ui.md).

## Blender
Always use Blender 5: `/home/gediminas.skucas/Apps/blender-5.2.2-linux-x64/blender`.
Never use the system `blender` (v4.0).
The `blender` MCP server needs Blender 5 running with the MCP add-on started (port 9876).

## Model remodelling
Models are rebuilt procedurally in Blender Python scripts. Tools, format conventions and per-model status: [docs/remodeling.md](docs/remodeling.md).

## Command whitelist
These commands do not need confirmation to run
* make tidy
* make tidy-fix
* make format
* make clean
* make

## Campaign levels
Every campaign level must pass the level checker with no warnings (`./levelcheck levels/lvl*`).
When a checker change flags a level, fix the level, also the hand-made levels 1 to 5. Manual placement is no reason
to keep a warning.

## Boss abilities
When a base monster gets a new ability (jump, ranged attack, swimming, ...), its boss gets one too (the boss whose
`kin` is that monster, `src/world/monster_kinds.cpp`). Make it stronger (farther, harder, faster) where that makes
sense. The same goes for draft plans: a plan that gives a base monster an ability covers its boss too.

## Plans
The directory "docs/plan/" is used to offload ideas (in *.md files) for a latter use.
For example, when the user says let's leave this idea for later, save info about it as "<idea-slug>.draft.<model>.md" inside the plan directory.
The model tag says who can implement it:
* `sonet`: simple, delegate to a Sonnet agent. Narrow and fully specified, follows an existing pattern (a potion,
  an amulet, a rename, a skill rule), no new Blender model or clip, no level design, no balance judgement.
* `opus`: complex, needs Opus. A new system or monster, new models or animation clips, level design, checker
  modelling, balance work, or several systems at once.
The default model stays Opus 5.5. When asked to implement a `sonet` draft, don't do it yourself and don't ask:
delegate it to a Sonnet 5.5 agent (Agent tool, `model: sonnet`), then report its result. Tell the agent to follow
AGENTS.md, run the checks, rename the plan when done and commit with a Sonnet 5.5 `Co-Authored-By` line.
`opus` drafts you implement yourself.
When refining a draft changes its size, rename it to the matching tag and update links to it.
Drafts are written for agents to implement later. Every draft is ready to implement as is; only a link to another
draft it needs first blocks it. Open points or several ideas in a draft are the implementer's choice.
Still, when reporting on drafts, warn about their open questions; when asked to refine a draft, list them.
When writing a draft, optimize for agent reading: the first lines (status) say what it needs first, if anything,
so readiness is clear without reading the whole file.
Once a draft plan is implemented, rename it without the "draft.<model>" part ("<idea-slug>.md") and update links to it.
When an implemented plan waits for the user's play-test, give the user a command that starts right at the feature,
so they don't play through the campaign: `./game --level N` (a campaign level that has it), or a script in
`tests/play/` (setup commands: `god`, `level`, `give`, `curse`, ...; committed, `make test` skips it) for
`./game --play tests/play/<plan-slug>.txt`. Not needed when the plan changes the campaign itself.
Once the user confirms an implemented plan is solved (tested / verified), move it to "docs/plan/solved/" and update links to it.
Exception: a plan that does not change the game (build, tooling, code style, docs, checks; no gameplay, visuals,
levels or balance) needs no user confirmation. Once implemented and its checks pass, move it straight to
"docs/plan/solved/" in the same commit.
When a plan moves to "docs/plan/solved/", turn the scenario tests it uses into unit tests (`tests/unit/`, docs/testing.md).
A scenario that needs a screenshot moves to "tests/scenarios/solved/": `make test` skips it, `make test SCENARIO=...`
still runs it as a slow regression check. Goal: lean, fast tests.
Code, tests and level files name a plan by its slug only (`// Climbers (plan monster-climbers): ...`), never by its
path or file name: a draft's rename or move to "solved/" then touches no code, so solving one needs no rebuild and
no test run. Only docs (markdown) link plans by path. Old path references in code: shorten them when you touch them.
New draft plans can arrive (and get committed) at any time, also in parallel with other work. They are not blocking:
ignore them, don't stop or change the current task because of them, and don't include them in your own commits.

## Git
In this project, commit your changes yourself when a task is done, without asking.
Trunk-based: `master` is the only long-lived branch and the only one that gets pushed.
Parallel tasks may run in a git worktree on a short-lived task branch, without asking. When the task is done:
1. Bring the branch up to date with `master` (merge or rebase the task branch), fix conflicts, rerun `make`, the
   style checks and the tests there.
2. Merge it into `master` in this main folder, not in the worktree: the user play-tests here. If the merge would
   touch files with uncommitted changes in the main folder, stop and ask.
3. Rebuild here (`make`), then remove the worktree and delete the task branch.

This overrides the global "no git actions" and "no worktrees" rules for these steps only.
Still ask before other git actions (push, rebase of `master`, config, other branches).

## Old save games
This game has no real players yet. Old saves can mostly be ignored.
