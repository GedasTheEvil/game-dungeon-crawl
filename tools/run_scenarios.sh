#!/bin/bash
# Runs scenario scripts through ./game, load-aware: a new one starts only while RESERVE_CORES cores (default 4) sit
# idle and MIN_FREE_MB of memory (default 4096) is available, at most JOBS at a time (default: nproc - RESERVE_CORES).
# One always runs. Each game renders with one llvmpipe thread and runs niced, so the desktop stays usable.
# Usage: tools/run_scenarios.sh [scenario.txt ...]
# Uses Xvfb when available, one server per run. HEADLESS=0 (or no Xvfb) runs one real window at a time, every frame
# drawn (SCENARIO_DRAW_ALL). Output: tests/out/<name>/.
cd "$(dirname "$0")/.." || exit 2

scenarios=("$@")
[ ${#scenarios[@]} -eq 0 ] && scenarios=(tests/scenarios/*.txt)

cores=$(nproc)
reserve=${RESERVE_CORES:-4}
min_free_mb=${MIN_FREE_MB:-4096}
jobs=${JOBS:-$((cores - reserve))}
[ "$jobs" -lt 1 ] && jobs=1
headless=0
if [ "${HEADLESS:-1}" != 0 ] && command -v xvfb-run >/dev/null; then
	headless=1
else
	export SCENARIO_DRAW_ALL=1
	jobs=1
fi
# Software GL under Xvfb starts a render thread per core in every game; one each is plenty for 800x600.
export LP_NUM_THREADS=${LP_NUM_THREADS:-1}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# Samples the system over one second: sets idle_x10 (idle cores x10) and free_mb (MemAvailable).
read_cpu() {
	local _ user nice sys idle iowait irq softirq steal
	read -r _ user nice sys idle iowait irq softirq steal _ </proc/stat
	cpu_idle=$((idle + iowait))
	cpu_total=$((user + nice + sys + idle + iowait + irq + softirq + steal))
}
sample() {
	local idle0 total0
	read_cpu
	idle0=$cpu_idle total0=$cpu_total
	sleep 1
	read_cpu
	idle_x10=$(((cpu_idle - idle0) * cores * 10 / (cpu_total - total0 + 1)))
	free_mb=$(($(awk '/^MemAvailable:/ {print $2}' /proc/meminfo) / 1024))
	[ "$idle_x10" -lt "$min_idle_x10" ] && min_idle_x10=$idle_x10
	[ "$free_mb" -lt "$min_free_seen" ] && min_free_seen=$free_mb
}
min_idle_x10=$((cores * 10)) min_free_seen=999999 peak=0 held=0

# $1 scenario, $2 its index (a distinct display number, so parallel xvfb-run -a does not race for the same one)
run_one() {
	local out code
	if [ $headless = 1 ]; then
		out=$(nice -n 10 xvfb-run -a -n $((200 + $2 * 3)) -s "-screen 0 800x600x24" ./game "$1" 2>&1)
	else
		out=$(./game "$1" 2>&1)
	fi
	code=$?
	[ $code -ne 0 ] && out+=$'\n'"   exit $code"
	echo $code >"$tmp/$2.code"
	{
		flock 9
		printf '== %s\n%s\n' "$1" "$out"
	} 9>"$tmp/print.lock"
}

for i in "${!scenarios[@]}"; do
	while :; do
		running=$(jobs -rp | wc -l)
		[ "$running" -eq 0 ] && break
		if [ "$running" -ge "$jobs" ]; then
			wait -n
			continue
		fi
		sample
		[ "$idle_x10" -ge $((reserve * 10)) ] && [ "$free_mb" -ge "$min_free_mb" ] && break
		held=$((held + 1))
	done
	run_one "${scenarios[$i]}" "$i" &
	running=$(jobs -rp | wc -l)
	[ "$running" -gt "$peak" ] && peak=$running
	# Let the new game load before the next sample sees its CPU and memory.
	[ "$jobs" -gt 1 ] && sleep 2
done
wait

failed=0
for i in "${!scenarios[@]}"; do
	[ "$(cat "$tmp/$i.code" 2>/dev/null)" = 0 ] || failed=$((failed + 1))
done

samples="min idle $((min_idle_x10 / 10)).$((min_idle_x10 % 10))/$cores cores, min free ${min_free_seen} MB"
[ "$min_free_seen" = 999999 ] && samples="no load samples"
echo "== load: peak $peak/$jobs at once, held back ${held}s, $samples"
echo "== $((${#scenarios[@]} - failed))/${#scenarios[@]} scenarios passed"
[ $failed -eq 0 ]
