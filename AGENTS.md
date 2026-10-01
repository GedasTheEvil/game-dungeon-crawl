## Process
After writing the code, verify it compiles by running `make`.
Once in compiles, run the code style checks `make format` and `make tidy`.

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
These commands do need confirmation to run
* make tidy
* make tidy-fix
* make format
* make clean
* make

## Campaign levels
Every campaign level must pass the level checker with no warnings (`./levelcheck levels/lvl*`).
When a checker change flags a level, fix the level, also the hand-made levels 1 to 5. Manual placement is no reason
to keep a warning.

## Plans
The directory "docs/plan/" is used to offload ideas (in *.md files) for a latter use.
For example, when the user says let's leave this idea for later, save info about it as "<idea-slug>.draft.md" inside the plan directory.
Once a draft plan is implemented, rename it without the "draft" part ("<idea-slug>.md") and update links to it.
Once the user confirms an implemented plan is solved (tested / verified), move it to "docs/plan/solved/" and update links to it.
New draft plans can arrive (and get committed) at any time, also in parallel with other work. They are not blocking:
ignore them, don't stop or change the current task because of them, and don't include them in your own commits.

## Git
In this project, commit your changes yourself when a task is done, without asking.
This overrides the global "no git actions" rule for commits only.
Still ask before other git actions (branches, rebase, push, config).
