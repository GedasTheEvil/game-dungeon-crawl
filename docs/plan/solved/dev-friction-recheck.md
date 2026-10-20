# Dev friction recheck: rules and structure that make agents redo work

Status: implemented 2026-10-20, solved (tooling, docs and structure only). See
[Implemented](#implemented-2026-10-20). Draft 2026-10-20, from the user. A re-run of
[dev-friction-review](dev-friction-review.md) (2026-10-12), which asks for one "from time to time". Tooling,
docs and structure only: no user check needed (docs/plans.md, Solved exception).

## Goal

Find what agents keep stepping on, and remove the cause. Typical: a rule (code style, doc style, plan bookkeeping, a
check) that no longer pays for itself, so agents write something, get corrected (by a check, a review or the user),
and redo it. Also the reverse: a mistake that keeps coming back because no check catches it. Keep what catches bugs
or keeps the code easy to find; cut or automate the rest.

## Today (2026-10-20)

* Since the last review: 78 of 206 transcripts, 177 commits. `src/` ~31.6k lines (24k on 2026-10-12), `src/world/`
  77 files (54). Largest: `ui/journal_view.cpp` 1253, `ui/menu.cpp` 1077, `ui/inventory.cpp` 987,
  `world/monster_kinds.cpp` 973. [split-god-objects](split-god-objects.md) is done.
* Agent rules are spread over `AGENTS.md` (84 lines, 7 commits of its own since 2026-10-12) and the docs it links
  (`docs/plans.md`, `development.md`, `testing.md`, `ui.md`, `glossary.md`, `modeling.md`, ~2000 lines in all).
* Plan bookkeeping is a large share of the commits: 21 `Solved:` commits, many `Draft ...` / `Plan ...` ones. Each
  rename or move updates links in other plans and docs; each solve turns scenarios into unit tests or parks them.
* One implementation was reverted outright (skill-flaming-arrows, then redone).

## Work

### 0. Evidence first: sessions since 2026-10-12

Same method as last time (transcripts in `~/.claude/projects/-home-gediminas-skucas-Apps-game-dungeon-crawl/`,
counted by a script or subagents; only counts and check names go into the commit, no transcript text). Look for:

* **Redo loops:** the same file edited again after a failed check, a review note or a user correction. Group by cause:
  tidy check (which one), format, layer check, levelcheck, docs-sync / unit test, glossary, plan naming or links, a
  user "no, not like that".
* **User corrections:** what the user had to say twice or more across sessions. Each repeated one is a rule that
  is missing, unclear, or in the wrong place (a doc agents do not read).
* **Rules agents broke:** e.g. a plan path in code, a missing boss ability, a level warning, a hand format, a `sonet`
  draft done by Opus. Is the rule hard to find, or unchecked?
* **Bookkeeping cost:** tool calls spent on plan renames, link updates, scenario-to-unit conversions, glossary rows,
  docs updates per feature. Compare with the feature work itself.
* **Check cost:** `make check` / `make tidy` / `make test` run times and fail counts now, against the 2026-10-12 table.
* **Search:** files read but not edited most often; files co-edited most often (last time `dungeon.h` +
  `dungeon_monsters.cpp`). Did split-god-objects lower them?

Put a short table of the findings here, then decide parts 1 to 3 from it. Save the script under `tools/` (or say in
this plan where it lives) so the next recheck reruns it instead of writing it again.

### 1. Rules: keep, automate or drop

Go over every rule an agent must follow (AGENTS.md, docs/plans.md, development.md coding rules, `.clang-tidy`,
`.clang-format`, levelcheck, glossary, docs-sync tests). For each, from the evidence:

* **Catches real mistakes, cheap:** keep.
* **Catches real mistakes, but agents keep missing it:** turn it into a check (script, test, tidy rule) that fails
  with a message saying the fix, instead of more text. E.g. a grep for plan paths in `src/` and `tests/`, a check that
  every boss's `kin` ability set covers its base monster's.
* **Mechanical:** automate it. E.g. a script that renames/moves a plan and rewrites the links to it; one that parks a
  scenario.
* **Taste only, or outdated:** drop it, and say so where it was.

Candidates to look at (from the commit log, check against the evidence): plan link updates on every rename, the
scenario-to-unit-test step on solve, tidy naming and narrowing checks (still the top fires last time), the doc
writing style, glossary upkeep, docs that repeat AGENTS.md.

### 2. Agent docs: one place per rule

* Each rule lives in one place; others link it. AGENTS.md stays short: the rules every task needs. Rules for one kind
  of task (plans, levels, models, UI) sit in that task's doc, linked from AGENTS.md.
* Check the where-is-X map in [development.md](../../development.md#where-is-x) against the current tree; fix the stale
  rows. Add the hot spots the search counts show.
* Remove instructions that describe what a check already enforces in full (the check's message is enough).

### 3. Structure: what hurts now

Same questions as last time (find by name, one feature few files, big files), on today's tree. In particular:

* `monster_kinds.cpp` (973 lines) after the data-tables work: one row per kind, or does each new monster still touch
  many places? Count the files a recent monster touched (cursed-mimic, living-ushabti, camel-spider).
* The big UI files: still one screen each, or did they grow past that (journal_view +160 lines since 2026-10-12)?
* Anything new that is spread out for no reason.

Small refactors that clearly pay off: do them here. Larger ones: one draft each, ranked by cost vs gain.

## Choices left to the implementer

All decided from the evidence of part 0; write down why in this plan when done: which rules to drop, which to turn
into checks, which refactors to do now, which become drafts.

## Checks

* `make check`, `make test`; `bin/levelcheck content/levels/lvl*` with no warnings.
* No behaviour change: unit tests and scenarios pass unchanged.
* A whole-tree reformat (if a format option changes) goes in its own commit, listed in `.git-blame-ignore-revs`.

## Implemented (2026-10-20)

### Evidence (part 0)

`python3 -I tools/friction_report.py --since 2026-10-12` (rerun it next time; `--prompts FILE` writes the typed
prompts for a read by hand). 57 sessions, 21 subagent threads, 4020 tool calls.

| Check | Runs (sessions) | Failed | Median s | p90 s | Total h |
|---|---|---|---|---|---|
| `make check` | 108 (35) | 14 | 124 | 304 | 4.4 |
| `make tidy` | 46 (14) | 16 | 185 | 326 | 2.5 |
| `make format-check` | 11 (6) | 6 | 16 | 132 | 0.2 |
| `make test` | 158 (33) | 15 | 16 | 66 | 1.3 |
| `make unit` | 111 (21) | 47 | 9 | 40 | 0.5 |
| `levelcheck` | 82 (19) | 2 | 1 | 6 | 0.2 |

* **Tidy is no longer the cost.** Fires (unique file:line per session): pragma-once 41 (7 sessions, all on
  2026-10-12, before its fix), unchecked-optional-access 20 (10), integer-division 7 (5), three more once each. No
  "not mine" noise since. All kept: each fire was a real, cheap fix.
* **Unit test fails** are the feature's own tests in the making (sim, monster rules, per-monster tests); docs sync
  (`docs_test.cpp`) 19 and golden dumps 15 fail with a message that says the fix. Kept.
* **Plan bookkeeping is the main cost.** 535 tool calls edited plan files, 322 grepped for plan links, 66 `git mv`d
  plans, against 322 that edited `src/`. In the commits: 405 plan file changes vs 774 in `src/`; a solve commit
  touched 3 to 9 files to fix links. Still, 36 links were broken on 2026-10-20 (and 3 more outside plans).
* **The user's prompts** (197): "draft status" / "what next" opens about 20 sessions; each agent then read every
  draft. Twice the user had to say a plan was already solved. One Sonnet pair of skills was left half done and needed
  a review round; one other agent's implementation was reverted.
* **Noise:** each `make check` printed the whole `clang-format -i` line, ~300 file names, into the agent's context.
* **Search and co-edits** are low after split-god-objects: the top `src/` pair co-edited in 3 sessions
  (`dungeon.h` + `dungeon_monsters.cpp` 29 before). A monster with new behaviour still touches 13 to 17 `src/` files
  (cursed-mimic 17, living-ushabti 13, monster-attack-reach 13), most for real reasons (its locomotion, clips, AI).
  `journal_view.cpp` was in it only for the creature's one sentence.

### Decided

* **`tools/plan.py`:** `status` (drafts oldest first: ready, blocked by an unsolved plan, on hold; implemented plans
  waiting for a play-test), `move <slug> <stage>` (git mv, rewrites every link to it and in it, lists its scenarios
  on solve), `links` (rewrites links to plans that moved, fails on one it cannot fix). `make check` runs `links`
  first. AGENTS.md and docs/plans.md say to use it, never to rename by hand; the status line format it reads is in
  docs/plans.md. Fixed the 39 broken links.
* **Quiet `make check`:** no command echo for format and the sub-makes.
* **Sonnet delegation:** the agent goes through the draft point by point before it renames the plan and reports what
  it left out (AGENTS.md).
* **Journal words out of the screen:** `creatureMoveNote` and the field notes moved from `ui/journal_view.cpp` to
  `world/journal_text.*` (level library, unit-testable). A new monster's sentence or a new note no longer touches the
  screen. Where-is-X updated; its paths all exist.
* **Kept:** all tidy checks, format settings, docs sync tests, golden dumps, glossary (most edited doc, 21 sessions,
  34 calls: cheap, and the docs test keeps it honest), the layer checks.
* **No larger refactor due.** `journal_view.cpp` (now ~1080 lines), `menu.cpp`, `inventory.cpp` stay one screen each.
