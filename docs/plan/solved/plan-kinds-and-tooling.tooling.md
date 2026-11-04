# Plan kinds and the plan tool's new steps

Status: solved 2026-11-04 (tooling, no play-test). From the user: mark plans as bugs or features in the file name,
and remove the friction found in the history of implementing drafts.

## Friction found

From about 400 commits and 210 session transcripts (2026-10-05 to 2026-11-04):

* Solving was a manual ritual after `plan.py move solved`: the "(play-tested by the user)" note by `sed`, the
  scenario's `git mv` to `tests/scenarios/solved/` and its links, the play script's removal (median 4 tool calls per
  "solved", up to 90). 74 of 77 play scripts still belonged to solved plans.
* The user had to ask what and how to play-test; 3 of the 5 plans waiting for one had no play script, none had a
  checklist. One waited 4 days.
* About 12 failed play-tests against about 89 implementations, fixed in place with no record.
* Bugs had no marker (about 20 to 30 of 256 plans were bug fixes, by their names and status lines).
* 13 drafts, all "ready", with nothing to choose by when the user asked for a small or a long task.
* Stale Status lines: 3 implemented plans still said "draft"; a won't-do plan moved by hand kept its draft name and
  no reason.
* Sonnet worktrees cut from a stale `origin/master`; definition-of-done gaps in delegated work.

## Done

* Kind in the file name: `<slug>.<kind>.draft.<model>.md` → `<slug>.<kind>.md` → `solved/<slug>.<kind>.md`, kind
  `bug`, `feature`, `tooling` (none: an old plan, a feature). `move <slug> kind-...` retags. A tooling plan goes
  straight to solved.
* Size (`size S|M|L`, required in drafts) and `prio high|low` in the Status line.
* `tools/plan.py`: `status` (rework, play-test with days waiting, drafts by kind with size and prio, "code names it
  already"), `next`, `playtest`, `new`, `solve`, `reject`, `verdict`, `hold` / `unhold`, `needs` / `unneed`,
  `done-check` (run by `move implemented`), `lint` (in `make check`), `board`. `move` now rewrites every relative
  link in the plan (scenarios, levels, images), not only links to plans, and the Status line on implement.
* `tools/plan_test.py`, in `make check`: the commands in a throwaway git repo.
* The open plans retagged and sized; the 3 stale Status lines fixed; play-test checklists for the 5 waiting plans;
  the hand-moved won't-do plan renamed and given its reason; the 75 play scripts of solved plans removed.
* Docs: [plans.md](../../plans.md), AGENTS.md, [version-control.md](../../version-control.md#worktrees) (worktrees
  from the local `master`), [testing.md](../../testing.md), [development.md](../../development.md), the glossary.
