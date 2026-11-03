#!/bin/bash
# Runs scenario scripts through bin/game, each through the shared load gate (tools/load_gate.sh: JOBS, RESERVE_CORES,
# MIN_FREE_MB, MIN_SWAP_FREE_MB). A game counts as still loading for GATE_LOAD_S seconds (default 15), with
# GATE_LOAD_MB (default 1500) more memory to come. Each game renders with one llvmpipe thread and runs niced, so the
# desktop stays usable.
# Usage: tools/run_scenarios.sh [scenario.txt ...]
# Uses Xvfb when available, one server per game. HEADLESS=0 (or no Xvfb) runs one real window at a time, every frame
# drawn (SCENARIO_DRAW_ALL). Output: tests/out/<name>/.
# SCENARIO_TIMES=file: one tab-separated line per scenario (name, s waited at the gate, s run, ticks, result), slowest
# first (docs/performance-testing.md).
cd "$(dirname "$0")/.." || exit 2

# Inside the gate: notes when the game starts (the end of its wait), then becomes it.
if [ "$1" = --stamp ]; then
	date +%s.%N >"$2"
	shift 2
	exec "$@"
fi

# $1 scenario, $2 queued, $3 started, $4 done (epoch seconds): its SCENARIO_TIMES line.
times_line() {
	local name result ticks
	name=$(basename "$1" .txt)
	result=$(tail -n 1 "tests/out/$name/result.txt" 2>/dev/null)
	ticks=$(grep -o '^tick [0-9]*' "tests/out/$name/result.txt" 2>/dev/null | tail -n 1)
	awk -v n="$name" -v q="$2" -v s="$3" -v d="$4" -v t="${ticks#tick }" -v r="${result%% *}" \
		'BEGIN { printf "%s\t%.1f\t%.1f\t%d\t%s\n", n, s - q, d - s, t, r }'
}

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
	local out code queued started=''
	queued=$(date +%s.%N)
	if [ $headless = 1 ]; then
		out=$(tools/load_gate.sh run tools/run_scenarios.sh --stamp "$tmp/$2.start" nice -n 10 xvfb-run -a -n $((200 + $2 * 3)) -s "-screen 0 800x600x24" bin/game "$1" 2>&1)
	else
		out=$(tools/load_gate.sh run tools/run_scenarios.sh --stamp "$tmp/$2.start" bin/game "$1" 2>&1)
	fi
	code=$?
	[ -s "$tmp/$2.start" ] && started=$(cat "$tmp/$2.start")
	[ -n "$SCENARIO_TIMES" ] && times_line "$1" "$queued" "${started:-$queued}" "$(date +%s.%N)" >"$tmp/$2.time"
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

if [ -n "$SCENARIO_TIMES" ]; then
	{
		printf 'scenario\twait_s\trun_s\tticks\tresult\n'
		cat "$tmp"/*.time 2>/dev/null | sort -t$'\t' -k3 -rn
	} >"$SCENARIO_TIMES"
	awk -F'\t' 'NR > 1 { w += $2; r += $3; t += $4 }
		END { if (t) printf "== times: %.0f s run, %.0f s waited, %d ticks, %.2f ms a tick -> %s\n", r, w, t, r * 1000 / t, f }' \
		f="$SCENARIO_TIMES" "$SCENARIO_TIMES"
fi
tools/load_gate.sh report scenarios
echo "== $((${#scenarios[@]} - failed))/${#scenarios[@]} scenarios passed"
[ $failed -eq 0 ]
