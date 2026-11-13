# Version control

Git in this repo. What an agent may do without asking (commits, worktrees) and what it must ask for:
[AGENTS.md](../AGENTS.md#git).

## Branches and commits

Trunk-based: `master` is the only long-lived branch and the only one that gets pushed. Run `make check` before each
commit. Opus and Fable models push `master` without asking, from the main folder only: after a commit there, or after step 2
below. Never from inside a worktree, never a task branch. Other models ask first.

## Worktrees

Parallel tasks may run in a git worktree on a short-lived task branch, without asking. Start it from the local
`master` (`git worktree add -b <task> ../<task> master`), never `origin/master`: that can be commits behind, and a
plan it needs can look unsolved there. When the task is done:
1. Rebase the task branch onto `master` (no need to ask), fix conflicts, rerun `make`, the style checks and the
   tests there.
2. Merge it into `master` in this main folder, not in the worktree: the user play-tests here. If the merge would
   touch files with uncommitted changes in the main folder, stop and ask.
3. Rebuild here (`make`), then remove the worktree and delete the task branch.
4. Opus and Fable models: push `master` from here.

## Blame

The whole-tree reformats are in `.git-blame-ignore-revs`; `git config blame.ignoreRevsFile .git-blame-ignore-revs`
makes `git blame` skip them.
