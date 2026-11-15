# A job tool for long commands: start, wait with a deadline, never a stuck loop

Status: draft 2026-11-15, size M, needs nothing first.

## Problem

Agents wait for `make check`, `make test` and worktree builds with hand-written shell loops
(`until grep -q X log; do sleep N; done`). [stuck-processes.md](../stuck-processes.md) already says not to. They still do,
and the loops get stuck. Today's case (skill-bleed's Sonnet agent): four `until grep -q "^done" /tmp/skill-bleed-check.log`
shells ran for an hour after the agent was finished. The `make check` behind the log had failed, so `done` never came. The
user found them in the background shells list. The lead agent had reported "nothing running" because its `ps` filter only
listed make, compilers and `bin/`.

Mined from the 181 transcripts of this project (main sessions and subagents, `~/.claude/projects/...`):

* 243 wait loops in 52 transcripts. By model: Sonnet 209, Opus 29. 137 had no `timeout`.
* The condition each loop waits on:

  | Loop condition | Loops |
  |---|---|
  | `grep -q MARKER log` | 132 |
  | `pgrep` | 47 |
  | `[ -s file ]` | 38 |
  | other (`kill -0 PID`, ...) | 26 |
* How they ended: 182 ended in the foreground. 29 hit the Bash tool's foreground timeout. 24 ran in the background and
  were notified. 6 ran in the background and never ended. 2 were denied.
* A foreground loop that times out is **moved to the background, not killed**: the harness keeps it running. Most of the
  leftovers come from here. Today's four started as two foreground loops plus their retries.

The ways a loop never ends:

1. **The marker only comes on success.** The producer writes `make check > log && echo done >> log` (or the agent greps
   for `passed`). A failed run never writes it.
2. **`pgrep -f PATTERN` matches the waiting shell itself**, whose command line holds the pattern. `until ! pgrep -f "make
   check"` never ends (6 of the never-notified ones).
3. **The pid or file belongs to something else.** A worktree removed under the loop, or a log path reused by the next run.
4. **No deadline.** Nothing stops the loop once the condition can no longer come true.

The harness already tells an agent when its own background command ends. The loops exist because agents want to block
until the result is there, or don't trust that the notification will come (a subagent waiting for a build it started in
a previous turn).

## Plan

One small tool in the place of every hand-written loop: `tools/job.sh`, plain bash, no dependencies.

* **Where the jobs live (decided, user 2026-11-15):** the project's own `tmp/` directory, `tmp/jobs/NAME/`. Add `/tmp/`
  to `.gitignore`. The main folder's `tmp/` holds every job, worktrees included: the tool finds it with `git rev-parse
  --git-common-dir`, whose parent is the main folder. So a job outlives `git worktree remove` (`reap` needs that), and
  `status` sees the jobs of all worktrees. Agents' other scratch files (logs, crops, test output) may go to `tmp/` too, in
  place of `/tmp/<name>`. `make clean` leaves it alone; `tools/job.sh clean` drops finished jobs older than a day.
* `tools/job.sh start NAME -- CMD...` runs CMD detached (`setsid`) in the current directory. It writes these files to
  `tmp/jobs/NAME/`:
  * `log`: stdout and stderr.
  * `pid`
  * `cwd`
  * `cmd`
  * `exit`: the exit code, written always. A wrapper `bash -c 'CMD; echo $? > exit'` with a `trap` for EXIT, INT and
    TERM writes it on success, failure or a kill.

  It refuses a NAME that is still running. A finished NAME is moved to `NAME.prev`. It prints one line: name, pid, log
  path.
* `tools/job.sh wait NAME [SECONDS]` waits until the `exit` file exists, at most SECONDS (default 100, under the Bash
  tool's 120 s foreground timeout; capped at 590 for `run_in_background`). It never waits forever and never uses `pgrep`.
  It polls the `exit` file and `kill -0` on the saved pid each second.
  * **Done:** prints the exit code and the last 15 lines of the log (`--tail N`), and exits with the job's code.
  * **Deadline reached:** prints `running: NAME, 4m10s, log 1234 lines, last: <last line>` and exits 124. The agent calls
    `wait` again or does other work. Nothing is left behind.
  * **Pid gone, no `exit` file** (killed hard, machine slept): prints `died` and exits 125.
* `tools/job.sh status [NAME]`: every job with its state, age and log size. `tools/job.sh log NAME [N]`: the last N
  lines.
* `tools/job.sh reap`: lists, then with `--kill` kills, the stray waits of this user:
  * Shells from `~/.claude/shell-snapshots` whose command holds `until`/`while` with `sleep`, older than 15 minutes.
  * Processes whose cwd is a removed worktree (`(deleted)`).
  * Running jobs whose cwd no longer exists.

  It replaces the `ps | grep` line in [stuck-processes.md](../stuck-processes.md#finding-leftovers): agents run it before
  they report "nothing running", and before `git worktree remove`.
* Make targets keep working as is: `tools/job.sh start check -- make check` then `tools/job.sh wait check 590` with
  `run_in_background`, or a plain `make check` with `run_in_background`. The tool is for when the agent must block or
  check again later.

Rules and enforcement:

* AGENTS.md, Process: one line. "Waiting for a long command: `tools/job.sh` ([stuck-processes.md]); never write an
  `until`/`while ... sleep` loop."
* [stuck-processes.md](../stuck-processes.md): a "Waiting" section built on the tool, the four ways a loop never ends
  (above), and `reap`.
* A project hook (`.claude/settings.json`, PreToolUse on Bash) blocks a command that matches `(until|while) .*; *do
  .*sleep`. Its message: "use tools/job.sh wait (docs/stuck-processes.md)". The text alone has not stopped agents (209
  Sonnet loops), so the hook does. It reads the command from stdin as JSON and exits 2 with the message. It lives in
  `tools/hooks/no_wait_loops.sh`, with a test in `tools/hooks/test_no_wait_loops.sh` that `make check` runs.
* The sub-agent brief template ([plans.md](../plans.md#model-tag), the `sonet` delegation): name the tool and the hook,
  since a Sonnet agent wrote most of the loops.
* `tools/friction_report.py`: count wait loops and loops moved to the background per session, so we see whether the
  numbers drop.

Tests (`tools/job_test.sh`, run by `make check`, a few seconds):

* A command that succeeds, one that fails, and one killed with SIGTERM all write `exit`.
* `wait` with a short deadline on a `sleep 30` job returns 124 in time, and leaves no process behind.
* A job killed with `kill -9` gives `died`.
* `start` refuses a running name.
* `reap` finds a planted `until false; do sleep 1; done` shell and leaves other shells alone.
* The hook blocks a wait loop and lets `for f in *; do ...; done` and `sleep 2` alone through.

Open points (the implementer's choice):

* Whether `wait` also stops early when the log stops growing for a long time (a hung test). Probably not: `make check`
  stages can be silent for minutes.
* Whether the hook should also block `sleep N` alone above some N (a blind wait).

## Done when

* `tools/job.sh` with `start`, `wait`, `status`, `log`, `reap`, `clean`, and its tests in `make check`; `/tmp/` in
  `.gitignore`.
* The hook in `.claude/settings.json`, tested.
* AGENTS.md, stuck-processes.md, the delegation brief in plans.md and the glossary (row: "job", "wait loop") updated.
* `tools/friction_report.py` counts wait loops.
