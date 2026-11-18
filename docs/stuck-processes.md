# Stuck processes

Ways an agent's shell or game run hangs, and how to avoid them. Add each new case you hit.

## Waiting for a long command

Run a long command (`make check`, `make test`, a build in a worktree) as a background job: the harness says when it
ends. When you must block on it, or check it again in a later turn, use `tools/job.sh` (plan wait-job-tool):

```
tools/job.sh start check -- make check     # detached; log, pid, exit code in tmp/jobs/check/ of the main folder
tools/job.sh wait check 100                # at most 100 s (590 with run_in_background); exits with make's code
tools/job.sh status                        # every job: state, age, log lines; `log check 40`: its last lines
```

`wait` exits 124 when the deadline comes first (it prints the age and the log's last line; call it again or do other
work) and 125 when the job died without an exit code (`kill -9`, the machine slept). The exit code is written on
success, failure and a kill, so a wait always ends. `stop NAME` kills a job; `clean` drops finished ones older than a
day. Agents' other scratch files (logs, crops) go to the main folder's `tmp/` too (ignored by git).

Never write an `until`/`while ... sleep` loop: the project's Bash hook (`tools/hooks/no_wait_loops.sh`) blocks it
(a heredoc's body is not checked). The 243 loops in the transcripts (2026-11-15) got stuck in four ways:

1. The marker only comes on success: `make check > log && echo done >> log`, or a grep for `passed`. A failed run
   never writes it.
2. `pgrep -f "<pattern>"` also matches the shell that runs it, because the loop's own command line holds the pattern.
   `until ! pgrep -f "make check"; do sleep 3; done` never ends.
3. The pid or file belongs to something else: a worktree removed under the loop, a log path reused by the next run.
4. No deadline. And a foreground command that hits the Bash tool's timeout is moved to the background, not killed: the
   loop runs on for hours.

`cmd | tail -n N` prints nothing until `cmd` ends. An empty output during a 15-minute `make check` is normal, not a
hang. To follow progress, read the job's log (`tools/job.sh log NAME`).

## The game and the editor

`bin/game` plays music and ends on SIGTERM only between frames (SDL turns it into a quit, as the window's close
button; not while it loads). Run it as `xvfb-run -a timeout -k 5 N bin/game --no-sound ...`, with
`timeout` inside `xvfb-run` (AGENTS.md, Scenario tests). `bin/editor` has no sound and no flags: `SDL_VIDEO_DRIVER=x11
xvfb-run -a timeout -k 5 N bin/editor [level file]` (SDL3 prefers Wayland, and `WAYLAND_DISPLAY` outlives `xvfb-run`:
without the driver the window opens on the desktop). The same for `bin/model-viewer` without `--shot`.

## Finding leftovers

Before you report that nothing is running, and before you remove a worktree, run:

```
tools/job.sh reap            # lists; `reap --kill` kills them
```

It lists the wait loops in agent shells older than 15 minutes, every process whose working directory is a removed
worktree (`readlink /proc/<pid>/cwd` ends in `(deleted)`), and running jobs whose folder is gone. All of them belong to
finished work. The game and the editor: `ps -u "$(id -u)" -o pid,etime,args | grep -E 'bin/(game|editor)'`.
