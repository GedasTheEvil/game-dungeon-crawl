#!/usr/bin/env bash
# PreToolUse hook on Bash (.claude/settings.json, plan wait-job-tool): blocks a hand-written wait loop
# (`until ...; do sleep N; done`, `while ...; do ... sleep ...`). Such loops got stuck for hours when the thing they
# waited for never came; tools/job.sh waits with a deadline instead. Reads the hook's JSON on stdin; exit 2 blocks.
# A heredoc's body is text (a doc, a script written to a file), not run here: the lines from the first `<<` on are
# left out.
cmd=$(jq -r '.tool_input.command // empty' 2>/dev/null) || exit 0
if printf '%s\n' "$cmd" | awk '/<</ { sub(/<<.*/, ""); print; exit } { print }' | tr '\n' ';' |
	grep -Eq '(^|[^[:alnum:]_])(until|while)[[:space:]].*;[[:space:]]*do[[:space:];](.*[^[:alnum:]_])?sleep[[:space:]]'; then
	echo "Blocked: a wait loop (until/while ... sleep). Use tools/job.sh start/wait, or run_in_background (docs/stuck-processes.md)." >&2
	exit 2
fi
exit 0
