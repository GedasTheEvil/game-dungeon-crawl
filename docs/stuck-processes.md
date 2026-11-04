# Stuck processes

Ways an agent's shell or game run hangs, and how to avoid them. Add each new case you hit.

## Waiting for a long command

Run a long command (`make check`, `make test`, a build in a worktree) as a background job: the harness says when it
ends. Do not write a wait loop around it.

* `pgrep -f "<pattern>"` also matches the shell that runs it, because the loop's own command line holds the pattern.
  `until ! pgrep -f "make check"; do sleep 3; done` never ends. This left two shells looping for hours after their
  worktrees were gone.
* `cmd | tail -n N` prints nothing until `cmd` ends. An empty output file during a 15-minute `make check` is normal,
  not a hang. To follow progress, write to a log (`make check > /tmp/<name>.log 2>&1`) and read it.

## The game and the editor

`bin/game` ignores SIGTERM and plays music. Run it as `xvfb-run -a timeout -k 5 N bin/game --no-sound ...`, with
`timeout` inside `xvfb-run` (AGENTS.md, Scenario tests). The same goes for `bin/editor`.

## Finding leftovers

Before you report that nothing is running, list every shell, not only `make` and `bin/`:

```
ps -u "$(id -u)" -o pid,etime,args | grep -E 'claude/shell-snapshots|bin/(game|editor)|make' | grep -v grep
```

A process whose working directory is a removed worktree (`readlink /proc/<pid>/cwd` ends in `(deleted)`) belongs to
finished work: kill it. Before you remove a worktree, check that no process still runs in it.
