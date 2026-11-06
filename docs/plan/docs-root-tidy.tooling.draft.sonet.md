# Docs and repo root tidy-up

Status: draft 2026-11-06, size S, needs nothing first.

## Problem

Small loose ends from a project review on 2026-11-06:

* `docs/levels.md` (639 lines) holds four subjects: the level format, the campaign order, levelcheck, levelgen.
  An agent that only needs the format reads the whole file.
* `docs/remodeling.md` is the history of a finished remodel ("(history)" in its title). It is linked from AGENTS.md
  as if current.
* `docs/plan/old/more-bosses.draft.md` is a shelved draft outside the plan lifecycle (`old/`, no kind or model in
  its name). Its Status says "shelved 2026-10-09, the user plans no more bosses for now".
* `docs/slides/` (slides.odp, slides.pdf) is not explained anywhere. It is the presentation of the original game,
  from the user's Bachelor's thesis. It stays, for history.
* The root launchers `Play`, `LevelEditor`, `ViewModel` start the compiled game and tools in a release build for
  testing, on systems that have no dev tools (no make). They are not `make run-*` duplicates, and they stay.
  `LevelEditor` and `ViewModel` pass `"$@"`, `Play` does not, so `./Play --level ...` drops its arguments.
  Project layout in development.md does not list them.

## Plan

* Split `docs/levels.md`: `docs/level-format.md` (Level format, Single cells by hand), `docs/levels.md` keeps the
  campaign order, levelcheck, levelgen, Test levels from ASCII and Tests. Each file starts with a link to the
  other. Fix every link to the moved sections (`grep -rn 'levels.md#' docs AGENTS.md tools src tests readme.md`;
  `docs/plan/solved/` may keep old anchors).
* `docs/remodeling.md`: keep the file. In AGENTS.md (Modelling), say it is the history of the finished remodel,
  read for a model's notes only.
* Move `docs/plan/old/more-bosses.draft.md` to `wont-do/` with `tools/plan.py` (see docs/plans.md, Won't do). Use
  `plan.py move` or `reject`, never `git mv`. If plan.py cannot take a file from `old/`, report that and leave it.
* development.md, Project layout: add `docs/slides/`: the presentation of the original game (the user's Bachelor's
  thesis), kept for history, not maintained. Add the root launchers: `Play`, `LevelEditor`, `ViewModel`
  start `bin/game`, `bin/editor` and `bin/model-viewer` from the repo root, for a release build tested on a system
  without dev tools. Keep them plain `sh` with no make or dev-tool dependency.
* `Play`: pass `"$@"` to `bin/game`, like the other two.
* `make check` passes.
