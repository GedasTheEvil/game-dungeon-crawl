# Dev friction review: tidy rules, code style, architecture

Status: implemented 2026-10-12, solved (tooling and structure only, no user check needed). See
[Implemented](#implemented-2026-10-12). Follow-up: [split-god-objects](split-god-objects.md).

## Goal

Agents should spend their time on the task, not on the checks. Two parts:

1. **Checks that matter.** Keep the rules that catch bugs or keep the code readable. Drop or automate the ones that
   only cost time. How many spaces go before a trailing comment is not worth an agent's turn.
2. **Structure that helps later work.** Finding the code for an issue should be fast and give few false hits. A
   change to one feature should touch few files.

Re-run this review from time to time as the code grows (the [architecture review](solved/architecture-review.md)
of 2026-10-07 was the last one).

## Today (2026-10-12)

* `make format` (`.clang-format`: LLVM base, tabs, 120 columns), `make format-check` (`tools/check_format.sh`: is
  formatted and stable), `make tidy` (`.clang-tidy`: clang-analyzer, bugprone, performance, portability, a few
  readability and modernize checks, identifier naming) with `make layers` (`tools/check_layers.sh`,
  `tools/check_sim.sh`).
* Seen in this session: `make tidy` prints known warnings in files nobody touched, so every agent has to sort them
  from its own:
  * `portability-avoid-pragma-once` on `src/graphics/gl_includes.h`, repeated once per including dir;
  * `clang-analyzer-security.ArrayBound` at `src/world/level.h:168` (`LevelGrid::set`, already bounds-checked:
    a false positive) and `src/ui/ui_draw.cpp:184` (`TAB_NAMES`);
  * `bugprone-unchecked-optional-access` at `src/world/dungeon_arrows.cpp:187`.
  The Sonnet agent of the death-spikes task reported "a note in level.h, not mine" and moved on. It was right, but
  it had to check.
* Aligned trailing comments (`AlignTrailingComments`, LLVM default): adding one longer line re-indents the comments
  of its neighbours, so diffs show lines that did not change. The check passes after `make format`, but the diffs
  get noisy and the commits pick up unrelated lines.
* Structure: 172 files, ~24k lines in `src/`. Largest: `ui/journal_view.cpp` (~1090), `ui/inventory.cpp` (~970),
  `ui/menu.cpp` (~950). `world/` has 54 files, 14 of them `dungeon_*`; `Dungeon` (`world/dungeon.h`, ~350 lines)
  holds every feature's state and helpers. `Monster` holds every locomotion's state (`Flight`, `Leap`, `Burrow`,
  `Charge`, `Climb`).
  Same-name files in two dirs: `entities/particles.*` and `graphics/particles.*`.

## Work

### 0. Evidence first: past sessions

Base the choices on what actually cost time, not on taste. The past Claude Code sessions of this project are JSONL
transcripts in `~/.claude/projects/-home-gediminas-skucas-Apps-game-dungeon-crawl/` (153 on 2026-10-12, 137 of them
run `make tidy`). Look through many of them, not one: use a script (or subagents, a few transcripts each) to pull out
the tidy, format-check and layer-check failures, then count per check:

* how often it fired, and how many turns or edits the fix took;
* whether the fix changed what the code does (a real bug) or only how it looks;
* the warnings agents saw and left alone as "not mine".

The same pass can count searches that missed (a grep or glob, then reads of files that turned out not to matter) to
feed part 3. Put a short table of the findings in this plan, then decide parts 1 to 3 from it. The transcripts stay
local: quote only counts and check names, no transcript text, in the commit.

### 1. Tidy: zero noise

* `make tidy` must print nothing on a clean tree. Fix each known warning, or suppress it with a `NOLINT(check)` and a
  reason. Turn off a check where it only gives false hits here, and say why in `.clang-tidy`, as the file already
  does for Padding.
* Then make tidy fail on any warning (`WarningsAsErrors: '*'`, or a non-zero exit in the makefile). A clean tree stays
  clean, and an agent knows every warning it sees is its own.
* Go over the enabled groups: keep what catches bugs (clang-analyzer, bugprone, most of performance), and the
  naming rules (they help search). Drop a readability check if it only flags taste.
* Measure `make tidy`'s run time. If it is long, add a target or a mode that checks only the changed files
  (`git diff --name-only` against HEAD), and have AGENTS.md ask for that one. The full run stays for CI-like checks.

### 2. Format: automatic, never a task

* Formatting is the tool's job only: agents run `make format` and never format by hand or think about it. Put that
  in AGENTS.md, not just the commands.
* Set `.clang-format` options that keep diffs small: no alignment of trailing comments or consecutive assignments
  across lines (`AlignTrailingComments: false` or `Kind: Never`, `AlignConsecutiveAssignments: None`, ...). Then
  reformat the whole tree in one commit of its own (no other change in it), and put its hash in
  `.git-blame-ignore-revs`.
* Keep `format-check` (stability matters: unstable code flips on every run).
* Maybe: one target `make check` that runs `format`, `format-check`, `tidy` (changed files) and `unit` in order, so
  AGENTS.md's Process names one command.

### 3. Architecture: fast to find, few false hits

Review against these questions, then do the refactors that pay off. One draft per larger refactor if it does not fit
here, like after the last review.

* **Find by name.** Does a file's name say the feature (`dungeon_monsters.cpp` holds flyer goals, walker paths, boss
  updates, ...)? Would one file per feature (flyers, climbers, chargers, ...) cut false hits? No two files share a
  name (`particles.*`).
* **One feature, few files.** Take some recent features (the bat dive: `gameplay_config.h`, `monster.h`,
  `monster_ai.cpp`, `dungeon.h`, `dungeon_monsters.cpp`; a potion; an amulet) and count the files they touched. Where
  a feature is spread out for no reason, gather it.
* **God objects.** `Dungeon` and `Monster` grow with every feature. Split by feature where the split is clean
  (per-locomotion state and its update in its own file, a `Dungeon` part per mechanism), not for its own sake.
* **Big UI files.** `journal_view`, `inventory`, `menu` near 1000 lines each: split by screen part if the parts are
  separate.
* **Agent docs.** AGENTS.md and the docs it links point to the right files. A short map of "where is X" (feature →
  files) in `docs/development.md` may save more search time than a refactor. Keep it up to date in the same commit as
  a move.
* Keep the layer checks (`check_layers.sh`, `check_sim.sh`): they are cheap and catch real mistakes.

## Choices left to the implementer

The user trusts the implementer to decide all of these from the evidence (part 0). Not blocking; pick and write
down why in the plan when it is done:

* which tidy checks to drop, and whether tidy fails on warnings or only on new ones;
* which alignment options to turn off;
* which refactors from part 3 are worth it now, and which become drafts of their own.

## Checks

* `make`, `make format-check`, `make tidy` (silent on a clean tree), `make unit`, `make test`.
* The whole-tree reformat commit changes whitespace only (`git diff -w` on it is empty).
* No behaviour change: the unit tests and scenarios pass unchanged.

## Implemented (2026-10-12)

### Evidence (part 0)

208 transcripts (161 sessions, 47 subagent threads), counted by a script. Only counts here.

| Check | Runs (sessions) | Failed | Cost |
|---|---|---|---|
| `make tidy` | 467 (96) | 102 printed warnings | 452 timed runs: 16.6 h, median 83 s, p90 333 s, 5 hit the 600 s timeout; 405 tool calls in fix loops |
| `make format-check` | 203 (46) | 6 (4 unstable) | each fixed by `make format`; 146 times run right after `make format` |
| layers / sim | 424 (96, mostly via tidy) | 7 (4 sessions) | each a real layer break |

* Warnings agents called "not mine" / pre-existing: 25 times in 17 sessions. The noise: `pragma once` in
  `gl_includes.h` (6 sessions, 45 runs), the `LevelGrid::at` ArrayBound false positive (6 / 12), and three
  more since fixed.
* Per check (fires = unique file:line per session): identifier-naming 170 (21 sessions, cosmetic, fixed in about one
  edit), narrowing-conversions 76 (16), implicit-bool-conversion 44 (9), use-bool-literals 40 (8),
  enum-size 24 (12, never fixed), unchecked-optional-access 21 (12), implicit-widening 12, integer-division 11
  (10), the rest under 10.
* Hand-aligning whitespace: 2 edits in all sessions. Agents do not format by hand; the cost of alignment is diff
  noise, not turns.
* Search: median 21 searches per session. Most read-but-not-edited: `dungeon_io.cpp` and `input.cpp` (17 sessions
  each). Most edited: `dungeon.h` 46, `game_state.cpp` 40, `monster.h` 39 of 101 sessions; top co-edits
  `dungeon.h` + `dungeon_monsters.cpp` 29, `monster.h` + `dungeon.h` 28.

### Decided

* **Tidy fails on any warning, checks changed sources** (`tools/tidy.sh`). Run time was the cost, not the rules:
  `make tidy` checks only the sources changed since the merge base with master and the ones that include a changed
  header (through the build's `.d` files): 45 s for a header used everywhere, seconds for a `.cpp`. `make tidy-all`
  checks all (~150 s). Fail on all warnings, not only new ones: the tree is clean, so all warnings are new.
* **Checks dropped:** `performance-enum-size` only (fired in 12 sessions, never fixed, a byte per enum). Kept naming
  (helps search, fixed in one edit), narrowing, implicit-bool, bool-literals: cheap to fix, and with tidy silent on a
  clean tree they no longer pile up as "not mine".
* **Format:** `AlignTrailingComments: Never`, `AlignEscapedNewlines: DontAlign`; consecutive assignments,
  declarations and macros are not aligned in LLVM already. `AlignOperands` stays: it is continuation indent, not
  neighbour noise. Whole-tree reformat in its own commit, in `.git-blame-ignore-revs`.
* **`make check`** = build, format, format-check, tidy, unit. AGENTS.md's Process names that one command.
* **Architecture:** a where-is-X map in [development.md](../../development.md#where-is-x). `dungeon_io.cpp` split:
  loading and saving stay, interaction (pick-up, gates, travel, teleport) moves to `dungeon_interact.cpp`.
  `graphics/particles.*` → `graphics/particles_draw.*`. The sim library takes `dungeon_*.cpp` by wildcard.
  Splitting `Dungeon` and `Monster` by feature is its own draft: [split-god-objects](split-god-objects.md).
  The big UI files stay: each is one screen, not a search hot spot.
* Layer checks kept; they no longer echo their file lists.
