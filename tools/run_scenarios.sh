#!/bin/bash
# Runs scenario scripts through bin/game, each through the shared load gate (tools/load_gate.sh: JOBS, RESERVE_CORES,
# MIN_FREE_MB, MIN_SWAP_FREE_MB). A game counts as still loading for GATE_LOAD_S seconds (default 15), with
# GATE_LOAD_MB (default 1500) more memory to come. Each game renders with one llvmpipe thread and runs niced, so the
# desktop stays usable.
# Usage: tools/run_scenarios.sh [scenario.txt ...]
# Uses Xvfb when available, one server per game. HEADLESS=0 (or no Xvfb) runs one real window at a time, every frame
# drawn (SCENARIO_DRAW_ALL). Output: tests/out/<name>/.
# Headless, the scripts without a screenshot run in batches, one game per batch (as many batches as the job cap): the
# assets load once per batch, each script runs in a fork of the loaded game (Scenario::loadBatch, plan
# scenario-light-startup). A batched script that comes to a frame to draw (a menu, the inventory) runs again alone,
# after the batches. The others run one game each.
# SCENARIO_TIMES=file: one tab-separated line per scenario (name, s waited at the gate, s run, ticks, result), slowest
# first (docs/performance-testing.md). A batched script's wait is its batch's, its run time has no startup.
cd "$(dirname "$0")/.." || exit 2

# Inside the gate: notes when the game starts (the end of its wait), then becomes it.
if [ "$1" = --stamp ]; then
	date +%s.%N >"$2"
	shift 2
	exec "$@"
fi

# $1 scenario, $2 seconds waited at the gate, $3 seconds run: its SCENARIO_TIMES line.
times_line() {
	local name result ticks
	name=$(basename "$1" .txt)
	result=$(tail -n 1 "tests/out/$name/result.txt" 2>/dev/null)
	ticks=$(grep -o '^tick [0-9]*' "tests/out/$name/result.txt" 2>/dev/null | tail -n 1)
	awk -v n="$name" -v w="$2" -v s="$3" -v t="${ticks#tick }" -v r="${result%% *}" \
		'BEGIN { printf "%s\t%.1f\t%.1f\t%d\t%s\n", n, w, s, t, r }'
}

# $1 scenario, $2 its index, $3 exit code, $4 its output
record() {
	echo "$3" >"$tmp/$2.code"
	local out=$4
	[ "$3" -ne 0 ] && out+=$'\n'"   exit $3"
	{
		flock 9
		printf '== %s\n%s\n' "$1" "$out"
	} 9>"$tmp/print.lock"
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
	started=${started:-$queued}
	[ -n "$SCENARIO_TIMES" ] && times_line "$1" "$(awk -v a="$queued" -v b="$started" 'BEGIN { print b - a }')" \
		"$(awk -v a="$started" -v b="$(date +%s.%N)" 'BEGIN { print b - a }')" >"$tmp/$2.time"
	record "$1" "$2" "$code" "$out"
}

# $1 the batch's display index, then the scenarios' indexes. The game prints "batch: start PATH" before a script's
# output and "batch: exit CODE SECONDS" after it; a script it never reached keeps no code (counts as failed).
run_batch() {
	local display=$1 out code queued started='' wait line cur='' text='' extra='' i paths=()
	shift
	for i in "$@"; do
		paths+=("${scenarios[$i]}")
	done
	queued=$(date +%s.%N)
	out=$(tools/load_gate.sh run tools/run_scenarios.sh --stamp "$tmp/b$display.start" nice -n 10 xvfb-run -a -n $((200 + display * 3)) -s "-screen 0 800x600x24" bin/game "${paths[@]}" 2>&1)
	code=$?
	[ -s "$tmp/b$display.start" ] && started=$(cat "$tmp/b$display.start")
	wait=$(awk -v a="$queued" -v b="${started:-$queued}" 'BEGIN { print b - a }')
	while IFS= read -r line; do
		case $line in
		'batch: start '*)
			cur=${line#batch: start }
			text=''
			;;
		'batch: exit '*)
			read -r i secs <<<"${line#batch: exit }"
			if [ "$i" = 4 ]; then # came to a frame to draw: alone, after the batches
				echo "${index[$cur]}" >>"$tmp/alone"
			else
				[ -n "$SCENARIO_TIMES" ] && times_line "$cur" "$wait" "$secs" >"$tmp/${index[$cur]}.time"
				record "$cur" "${index[$cur]}" "$i" "${text%$'\n'}"
			fi
			cur=''
			;;
		*)
			if [ -n "$cur" ]; then
				text+=$line$'\n'
			else
				extra+=$line$'\n'
			fi
			;;
		esac
	done <<<"$out"
	[ -n "$cur" ] && extra+="$cur: no exit line"$'\n'$text
	if [ $code -ne 0 ] && [ -n "$extra" ]; then
		{
			flock 9
			printf '== batch of %d\n%s   exit %d\n' $# "$extra" $code
		} 9>"$tmp/print.lock"
	fi
}

# The scripts for batches (headless, no screenshot), longest first, each to the batch with the fewest lines so far.
declare -A index
singles=()
blind=()
for i in "${!scenarios[@]}"; do
	index[${scenarios[$i]}]=$i
	if [ $headless = 1 ] && ! grep -q '^[[:space:]]*screenshot' "${scenarios[$i]}"; then
		blind+=("$(wc -l <"${scenarios[$i]}") $i")
	else
		singles+=("$i")
	fi
done
batches=$jobs
[ ${#blind[@]} -lt "$batches" ] && batches=${#blind[@]}
[ ${#blind[@]} -eq 1 ] && singles+=("${blind[0]#* }") && batches=0
declare -a batch_lines batch_items
for ((b = 0; b < batches; b++)); do
	batch_lines[b]=0
	batch_items[b]=''
done
while read -r lines i; do
	[ -n "$i" ] || continue
	best=0
	for ((b = 1; b < batches; b++)); do
		[ "${batch_lines[b]}" -lt "${batch_lines[best]}" ] && best=$b
	done
	batch_lines[best]=$((batch_lines[best] + lines))
	batch_items[best]+=" $i"
done < <([ "$batches" -gt 0 ] && printf '%s\n' "${blind[@]}" | sort -rn)

# One more than the cap: the extra one waits at the gate, so a game starts as soon as the load allows. Batches first:
# they take the longest.
for ((b = 0; b < batches; b++)); do
	while [ "$(jobs -rp | wc -l)" -gt "$jobs" ]; do
		wait -n
	done
	# shellcheck disable=SC2086 # the indexes, split on purpose
	run_batch $((${#scenarios[@]} + b)) ${batch_items[b]} &
done
for i in "${singles[@]}"; do
	while [ "$(jobs -rp | wc -l)" -gt "$jobs" ]; do
		wait -n
	done
	run_one "${scenarios[$i]}" "$i" &
done
wait
# The batched scripts that came to a frame to draw (a menu, the inventory; a fork does not draw): each alone.
if [ -s "$tmp/alone" ]; then
	while read -r i; do
		while [ "$(jobs -rp | wc -l)" -gt "$jobs" ]; do
			wait -n
		done
		run_one "${scenarios[$i]}" "$i" &
	done <"$tmp/alone"
	wait
fi

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
