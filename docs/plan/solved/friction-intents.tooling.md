# Friction report by intent, plan lookup

Status: solved 2026-11-06. Was: draft 2026-11-06, size M, needs nothing first.

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

## Done (2026-11-06)

`friction_report.py` prints plan commands by intent (with 3 examples each) and unit failures by class and test file.
Numbers over the two weeks 2026-10-23 to 11-06 (94 sessions, 54 subagent threads):

| Plan intent | Commands |
|---|---|
| read a plan (cat, sed, head) | 195 |
| search in drafts (`ls docs/plan \| grep X`, grep in drafts) | 119 |
| `plan.py move` | 104 |
| `plan.py status` | 103 |
| `sed -i` on a plan | 48 |
| topic search in `solved/`, `wont-do/` | 40 |
| `plan.py` lint, verdict, done-check, new, links, playtest | 15, 13, 12, 10, 5, 2 |
| link search | 4 |
| status-line grep | 3 |

The old "plan link greps" count was mostly reads and topic searches: real link searches are 4, Status-line greps 3.

`make unit` (alone or in `make check`/`make test`) failures: 51 assertion, 24 crash, 13 other (no doctest line:
usually a build failure cut off by `tail`), 0 compile/link classified (they show as "other" or as a failing `make`).
Failing test files: `docs_test.cpp` 11, `stun_test.cpp` 7, `world_rules_test.cpp` 6, `golden_test.cpp` 4,
`subcell_monsters_test.cpp` 4, `giant_rat_call_test.cpp` 4. Failed in 3+ sessions that did not edit the test:
`docs_test.cpp` 8, `golden_test.cpp` 3, `world_rules_test.cpp` 3. These are not flaky: `docs_test` checks that the
docs list every monster, command and glossary name, `golden_test` and `world_rules_test` pin values a gameplay change
moves. They fail on purpose when code changes without its docs or goldens.

Step 4, acted on:

* Topic search (159 in two weeks): `tools/plan.py find TERM...` (plans.md, AGENTS.md, glossary "plan find", a test in
  `plan_test.py`).
* Status-line greps (3): too few, nothing to do.
* `sed -i` on plans (48, 24 a week): sampled; almost all are from before `plan.py solve`, `verdict` and the scenario
  parking existed (status "(play-tested by the user)", `tests/scenarios/solved/` paths). The one recent kind is the
  "Won't do DATE: reason" text before `move wont-do` (2 in two weeks): too few for a flag.
* Unit failures: no flaky test found; nothing for `check-speed`.
