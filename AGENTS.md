## Process
After writing the code, verify it compiles by running `make`.
Once in compiles, run the code style checks `make format`, `make format-check` and `make tidy`.

## Glossary
Unsure of a term, or naming a new thing: grep [docs/glossary.md](docs/glossary.md) (one row per term, no need to read it all). Add new terms there.

## Glyphs
Level glyphs (monsters, tiles) are only read by tools and agents. Pick a free one yourself; never ask the user.

## Scenario tests
To check game behaviour or visuals, write a script in `tests/scenarios/` and run `make test SCENARIO=...`. Screenshots and results go to `tests/out/<name>/`. Reference: [docs/testing.md](docs/testing.md).

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
For example, when the user says let's leave this idea for later, save info about it as "<idea-slug>.draft.md" inside the plan directory.
Drafts are written for agents to implement later. Every draft is ready to implement as is; only a link to another
draft it needs first blocks it. Open points or several ideas in a draft are the implementer's choice.
Still, when reporting on drafts, warn about their open questions; when asked to refine a draft, list them.
When writing a draft, optimize for agent reading: the first lines (status) say what it needs first, if anything,
so readiness is clear without reading the whole file.
Once a draft plan is implemented, rename it without the "draft" part ("<idea-slug>.md") and update links to it.
Once the user confirms an implemented plan is solved (tested / verified), move it to "docs/plan/solved/" and update links to it.
Exception: a plan that does not change the game (build, tooling, code style, docs, checks; no gameplay, visuals,
levels or balance) needs no user confirmation. Once implemented and its checks pass, move it straight to
"docs/plan/solved/" in the same commit.
When a plan moves to "docs/plan/solved/", turn the scenario tests it uses into unit tests (`tests/unit/`, docs/testing.md).
A scenario that needs a screenshot moves to "tests/scenarios/solved/": `make test` skips it, `make test SCENARIO=...`
still runs it as a slow regression check. Goal: lean, fast tests.
New draft plans can arrive (and get committed) at any time, also in parallel with other work. They are not blocking:
ignore them, don't stop or change the current task because of them, and don't include them in your own commits.

## Git
In this project, commit your changes yourself when a task is done, without asking.
This overrides the global "no git actions" rule for commits only.
Still ask before other git actions (branches, rebase, push, config).
