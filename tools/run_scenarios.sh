#!/bin/bash
# Runs scenario scripts through ./game, each through the shared load gate (tools/load_gate.sh: JOBS, RESERVE_CORES,
# MIN_FREE_MB, MIN_SWAP_FREE_MB). A game counts as still loading for GATE_LOAD_S seconds (default 15), with
# GATE_LOAD_MB (default 1500) more memory to come. Each game renders with one llvmpipe thread and runs niced, so the
# desktop stays usable.
# Usage: tools/run_scenarios.sh [scenario.txt ...]
# Uses Xvfb when available, one server per game. HEADLESS=0 (or no Xvfb) runs one real window at a time, every frame
# drawn (SCENARIO_DRAW_ALL). Output: tests/out/<name>/.
cd "$(dirname "$0")/.." || exit 2

scenarios=("$@")
[ ${#scenarios[@]} -eq 0 ] && scenarios=(tests/scenarios/*.txt)

headless=0
if [ "${HEADLESS:-1}" != 0 ] && command -v xvfb-run >/dev/null; then
	headless=1
else
	export SCENARIO_DRAW_ALL=1
	export JOBS=1
fi
jobs=$(tools/load_gate.sh jobs)
export GATE_LOAD_S=${GATE_LOAD_S:-15} GATE_LOAD_MB=${GATE_LOAD_MB:-1500}
# Software GL under Xvfb starts a render thread per core in every game; one each is plenty for 800x600.
export LP_NUM_THREADS=${LP_NUM_THREADS:-1}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

export GATE_STATS="$tmp/load"

# $1 scenario, $2 its index (a distinct display number, so parallel xvfb-run -a does not race for the same one)
run_one() {
	local out code
	if [ $headless = 1 ]; then
		out=$(tools/load_gate.sh run nice -n 10 xvfb-run -a -n $((200 + $2 * 3)) -s "-screen 0 800x600x24" ./game "$1" 2>&1)
	else
		out=$(tools/load_gate.sh run ./game "$1" 2>&1)
	fi
	code=$?
	[ $code -ne 0 ] && out+=$'\n'"   exit $code"
	echo $code >"$tmp/$2.code"
	{
		flock 9
		printf '== %s\n%s\n' "$1" "$out"
	} 9>"$tmp/print.lock"
}

# One more than the cap: the extra one waits at the gate, so a game starts as soon as the load allows.
for i in "${!scenarios[@]}"; do
	while [ "$(jobs -rp | wc -l)" -gt "$jobs" ]; do
		wait -n
	done
	run_one "${scenarios[$i]}" "$i" &
done
wait

failed=0
for i in "${!scenarios[@]}"; do
	[ "$(cat "$tmp/$i.code" 2>/dev/null)" = 0 ] || failed=$((failed + 1))
done

tools/load_gate.sh report scenarios
echo "== $((${#scenarios[@]} - failed))/${#scenarios[@]} scenarios passed"
[ $failed -eq 0 ]
