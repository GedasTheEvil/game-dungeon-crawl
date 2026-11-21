#!/usr/bin/env bash
# `make check`'s stages, timed (plan check-speed-stage-breakdown): build, plan, format, tidy, unit. One line per run is
# appended to the log, read by tools/friction_report.py:
#
#   <iso time> <ok|FAIL:stage> files=N tidied=N wait=S build=S plan=S format=S tidy=S unit=S total=S
#
# files: files changed since the merge base with master (committed, staged, unstaged, new). tidied: sources clang-tidy
# ran on (tools/tidy.sh). wait: seconds the run's jobs sat at the load gate, summed over jobs (tools/load_gate.sh). A
# stage that did not run is 0. Tidy and unit run at the same time (after format); each time is its own, total is wall
# time. Their output is printed after both end, tidy first. Both failed: FAIL:tidy. The log sits in the git common
# dir, so every worktree writes to the one file. CHECK_LOG overrides its path.
set -u
cd "$(dirname "$0")/.." || exit 2
log=${CHECK_LOG:-$(realpath "$(git rev-parse --git-common-dir)")/check-stages.log}
make=${MAKE:-make}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
export GATE_WAIT_FILE=$tmp/wait CHECK_STAGE_NOTE=$tmp/tidied
: >"$tmp/wait"

declare -A secs=([build]=0 [plan]=0 [format]=0 [tidy]=0 [unit]=0)
failed=''
start=$(date +%s.%N)

# $1: stage name, then the shell code of the stage. Stops the run at the first failure (the log line is written anyway).
stage() {
	[ -z "$failed" ] || return 0
	local name=$1 t0
	t0=$(date +%s.%N)
	bash -c "set -e; $2" || failed=$name
	secs[$name]=$(echo "$(date +%s.%N) - $t0" | bc)
}

base=$(git merge-base HEAD master 2>/dev/null || echo HEAD)
files=$({ git diff --name-only "$base"; git ls-files --others --exclude-standard; } | sort -u | grep -c .)

stage build "$make --no-print-directory all"
stage plan './tools/plan.py links; ./tools/plan.py lint; python3 -I tools/plan_test.py; python3 -I tools/docs_check.py; tools/job_test.sh; tools/install_check_test.sh; tools/hooks/test_no_wait_loops.sh'
stage format "$make --no-print-directory format; $make --no-print-directory format-check-changed"
# Tidy and unit (plan check-tidy-unit-parallel) do not depend on each other: run them together (both go through the
# load gate), each output in its own file, printed afterwards (tidy first) so it does not interleave. A failed earlier
# stage skips both.
if [ -z "$failed" ]; then
	par() { # $1: name, $2: make target
		local t0 rc=0
		t0=$(date +%s.%N)
		"$make" --no-print-directory "$2" >"$tmp/$1.out" 2>&1 || rc=$?
		echo "$(date +%s.%N) - $t0" | bc >"$tmp/$1.secs"
		echo "$rc" >"$tmp/$1.rc"
	}
	par tidy tidy &
	par unit unit &
	wait
	for name in tidy unit; do
		cat "$tmp/$name.out"
		secs[$name]=$(cat "$tmp/$name.secs")
		[ "$(cat "$tmp/$name.rc")" = 0 ] || failed=${failed:-$name}
	done
fi

total=$(echo "$(date +%s.%N) - $start" | bc)
wait_s=$(awk '{s += $1} END {printf "%.0f", s}' "$tmp/wait")
tidied=$(cat "$tmp/tidied" 2>/dev/null || echo 0)
printf '%s %s files=%d tidied=%d wait=%s build=%.0f plan=%.0f format=%.0f tidy=%.0f unit=%.0f total=%.0f\n' \
	"$(date +%Y-%m-%dT%H:%M:%S)" "${failed:+FAIL:}${failed:-ok}" "$files" "$tidied" "$wait_s" \
	"${secs[build]}" "${secs[plan]}" "${secs[format]}" "${secs[tidy]}" "${secs[unit]}" "$total" >>"$log"
[ -z "$failed" ]
