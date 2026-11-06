# Friction report by intent, plan lookup

Status: draft 2026-11-06, size M, needs nothing first.

## Problem

`tools/friction_report.py` counts are too coarse to say what to fix:

* "plan link greps" (291 in the week to 2026-11-06) counts every Bash command that mentions `grep` and
  `docs/plan`. A sample of this week's commands shows that most are not link searches: Status-line greps over the
  drafts, finding a solved plan by topic (`ls docs/plan/solved | grep -iE 'subcell|shape'`,
  `grep -ln 'ring.*upgrade' docs/plan/solved/*.md`), reads of a draft chained with an unrelated grep, and `sed -i`
  fix-ups of a Status line after `plan.py move`. 120 of 309 matching commands touch `solved/`.
* `make unit` failed in 41 of 89 runs, and those fix loops took a median of 3 calls (the most of any check). The
  report cannot tell a failing test written first on purpose from a regression, a compile error in a test, or a
  slow or flaky test.

## Plan

1. Report: split the plan bookkeeping by intent. Classify each command by its first plan-related part:
   status-line grep, topic search in `solved/`, topic search in drafts, link search, `sed -i` on a plan,
   `plan.py <command>`. Print one count per intent with 3 short examples each.
2. Report: classify `make unit` failures from the output: compile/link error, doctest assertion (which test file),
   crash/timeout. Count per class and per test file. Flag a test that fails in 3+ sessions without its file being
   edited in them (flaky or slow).
3. Run it over the last two weeks and write the numbers into this plan.
4. Act on the top items only, for example:
   * Topic search dominates: `tools/plan.py find <term>...` searches drafts, `solved/` and `wont-do/` (title, Status
     line, body, case-insensitive) and prints `state slug: title` plus the first matching line. Add it to plans.md
     and the glossary, with a test in `plan_test.py`.
   * Status-line greps dominate: `plan.py status` already prints the drafts. Check that `status --all` (or `show
     <slug>`) covers what the greps want.
   * `sed -i` on Status lines after a move: the `plan.py` step that should have written that text does it.
   * A flaky or slow unit test: fix it, or note it in `check-speed`.

## Open points

* How many of the steps in 4 to do depends on the numbers in 3. Skip a step whose count is small (< 10 a week).
