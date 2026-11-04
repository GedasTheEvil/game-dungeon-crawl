# Version control

Git in this repo. What an agent may do without asking (commits, worktrees) and what it must ask for:
[AGENTS.md](../AGENTS.md#git).

## Branches and commits

Trunk-based: `master` is the only long-lived branch and the only one that gets pushed. Run `make check` before each
commit. Opus models push `master` after each commit (or after the merge of a worktree task), without asking; other
models ask first.

## Worktrees

Parallel tasks may run in a git worktree on a short-lived task branch, without asking. When the task is done:
1. Rebase the task branch onto `master` (no need to ask), fix conflicts, rerun `make`, the style checks and the
   tests there.
2. Merge it into `master` in this main folder, not in the worktree: the user play-tests here. If the merge would
   touch files with uncommitted changes in the main folder, stop and ask.
3. Rebuild here (`make`), then remove the worktree and delete the task branch.

## Blame

The whole-tree reformats are in `.git-blame-ignore-revs`; `git config blame.ignoreRevsFile .git-blame-ignore-revs`
makes `git blame` skip them.
