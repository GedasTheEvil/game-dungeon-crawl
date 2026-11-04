# Editor: open a level file from the command line

Status: solved 2026-11-04. Was: draft 2026-11-01, needs nothing first. From the user.

## Problem

The level editor takes no arguments: `main` (`tools/editor/editor.cpp`) ignores `argv`. A level is loaded only by
typing its name into the name field and pressing `Load`, which reads `tools/editor/saved/<name>`. A campaign level
(`content/levels/lvlN`) cannot be opened in place: it has to be copied into `tools/editor/saved/` and copied back.

## Plan

* `bin/editor <file>` (the editor's binary, `$(EDITOR)` in the makefile) loads `<file>` at start, any path.
  No argument: as now, an empty map.
* Resolve a relative path against the caller's working directory: make it absolute before `Paths::enterRootDir()`
  (which changes into the repo root). Read the argument after `glutInit`, which strips GLUT's own options from `argv`.
* Load with the same code as `Editor::load` (`loadLevelFile`, reset `reportStale` and `checked`, status
  "Loaded from <path>"); split the path handling out of `load()` rather than duplicate it.
* The editor remembers the file it opened: `Save` (`Ctrl+S`) writes back to that path, the name field shows its file
  name (`lvl3`). Typing a new name in the field drops the remembered path: `Save` and `Load` then use
  `tools/editor/saved/<name>` as now.
* A file that does not load (missing, parse error): print `<path>: <error>` to stderr and exit 1, before the window
  opens if possible. More than one argument: print a usage line and exit 1.
* `make run-editor LEVEL=content/levels/lvl3` passes the file on (`$(EDITOR) $(LEVEL)`); no `LEVEL`, no argument.

## Docs

* `tools/editor/readme.md`: the first line (`make run-editor`) and the Workflow (step 6, copying to
  `content/levels/`, now optional: open the campaign file and save it in place). `tests/unit/docs_test.cpp` checks
  the readme; keep it passing.
* `docs/development.md` (the `make run-editor` line) if it lists the editor's options.

## Checks

* `make check`.
* Smoke test, silent and bounded (AGENTS.md): `xvfb-run -a timeout -k 5 5 bin/editor content/levels/lvl1` starts
  (timeout, exit 124); `bin/editor no/such/file` exits 1 with the error on stderr; a relative path from another
  directory loads.
