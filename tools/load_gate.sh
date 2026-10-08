#!/bin/bash
# One load gate for every heavy process (scenario games, clang-tidy, compiles), shared by all runs at once.
# Usage:
#   tools/load_gate.sh run CMD [ARG...]  waits for the gate, runs CMD, exits with its code
#   tools/load_gate.sh jobs              prints the job cap
#   tools/load_gate.sh report LABEL      prints the peaks of the run that wrote GATE_STATS, then removes the file
# A job starts only while fewer than JOBS jobs run (default: (nproc - RESERVE_CORES) * 4 / 5, at least 1),
# RESERVE_CORES cores (default 4) sit idle over a GATE_SAMPLE_S sample (default 0.5 s), MIN_FREE_MB of memory
# (default 4096) is available (MemAvailable) and MIN_SWAP_FREE_MB of swap (default 4096) is free. One always runs.
# A job started less than GATE_LOAD_S seconds ago (default 0) counts as still taking GATE_LOAD_MB more memory
# (default 0): a game loads for seconds, a sample right after its start does not see it all.
# Jobs run on the last GATE_CPUS cores (default: the job cap, one core per job): however much a job grows after its
# start, the first cores stay free for the desktop.
# The jobs of all runs count together: slot files under GATE_DIR, held by flock while the job runs.
# GATE_STATS (a file): each run appends its samples there, `report` sums them up.

cores=$(nproc)
reserve=${RESERVE_CORES:-4}
jobs=${JOBS:-$(((cores - reserve) * 4 / 5))}
[ "$jobs" -lt 1 ] && jobs=1
min_free_mb=${MIN_FREE_MB:-4096}
min_swap_mb=${MIN_SWAP_FREE_MB:-4096}
load_s=${GATE_LOAD_S:-0}
load_mb=${GATE_LOAD_MB:-0}
sample_s=${GATE_SAMPLE_S:-0.5}
cpus=${GATE_CPUS:-$jobs}
[ "$cpus" -lt 1 ] && cpus=1
[ "$cpus" -gt "$cores" ] && cpus=$cores
dir=${GATE_DIR:-/tmp/dungeon-crawl-gate-$(id -u)}

# $1: a stats line. The first line of a run is its start: time, available memory.
stat_line() {
	[ -n "$GATE_STATS" ] || return 0
	mkdir -p "$(dirname "$GATE_STATS")"
	if [ ! -s "$GATE_STATS" ]; then
		(set -o noclobber && echo "t $(date +%s) $(meminfo MemAvailable)" >"$GATE_STATS") 2>/dev/null
	fi
	echo "$1" >>"$GATE_STATS"
}

meminfo() { awk -v k="$1:" '$1 == k {print int($2 / 1024)}' /proc/meminfo; }

read_cpu() {
	local _ user nice sys idle iowait irq softirq steal
	read -r _ user nice sys idle iowait irq softirq steal _ </proc/stat
	cpu_idle=$((idle + iowait))
	cpu_total=$((user + nice + sys + idle + iowait + irq + softirq + steal))
}

# Samples the system over sample_s seconds: idle_x10 (idle cores x10), free_mb, swap_mb.
sample() {
	local idle0 total0
	read_cpu
	idle0=$cpu_idle total0=$cpu_total
	sleep "$sample_s"
	read_cpu
	idle_x10=$(((cpu_idle - idle0) * cores * 10 / (cpu_total - total0 + 1)))
	free_mb=$(meminfo MemAvailable)
	swap_mb=$(meminfo SwapFree)
	stat_line "s $idle_x10 $free_mb $swap_mb"
}

# Counts the jobs running (busy) and those started less than load_s ago (loading); holds the first free slot below
# the cap as fd $slot, its number $slot_k (empty: none).
scan_slots() {
	local f fd k now
	busy=0 loading=0 slot='' slot_k=''
	now=$(date +%s)
	for f in "$dir"/slot.*; do
		[ -e "$f" ] || continue
		k=${f##*.}
		exec {fd}>>"$f"
		if flock -n "$fd"; then
			if [ -z "$slot" ] && [ "$k" -lt "$jobs" ]; then
				slot=$fd slot_k=$k
				continue
			fi
		else
			busy=$((busy + 1))
			[ $((now - $(stat -c %Y "$f"))) -lt "$load_s" ] && loading=$((loading + 1))
		fi
		exec {fd}>&-
	done
}

run() {
	local lock k
	mkdir -p "$dir"
	for ((k = 0; k < jobs; k++)); do
		[ -e "$dir/slot.$k" ] || : >>"$dir/slot.$k"
	done
	# One starter at a time, so two cannot both see the same idle cores. Under it no one else scans, so a free slot
	# can stay held while sampling.
	exec {lock}>"$dir/start.lock"
	flock "$lock"
	while :; do
		scan_slots
		[ -n "$slot" ] && [ "$busy" -eq 0 ] && break
		sample # while all slots are taken, it only feeds the stats
		if [ -n "$slot" ]; then
			[ "$idle_x10" -ge $((reserve * 10)) ] && [ $((free_mb - loading * load_mb)) -ge "$min_free_mb" ] &&
				[ "$swap_mb" -ge "$min_swap_mb" ] && break
			stat_line h
			exec {slot}>&-
		fi
	done
	touch "$dir/slot.$slot_k"
	stat_line "p $((busy + 1)) $jobs"
	exec {lock}>&-
	taskset -c $((cores - cpus))-$((cores - 1)) "$@" {slot}>&-
}

report() {
	[ -n "$GATE_STATS" ] && [ -s "$GATE_STATS" ] || return 0
	awk -v label="$1" -v cores="$cores" -v sample_s="$sample_s" -v now="$(date +%s)" '
		$1 == "t" { start = $2; free0 = $3 }
		$1 == "s" {
			n++
			if (n == 1 || $2 < idle) idle = $2
			if (n == 1 || $3 < free) free = $3
			if (n == 1 || $4 < swap) swap = $4
		}
		$1 == "p" { if ($2 > peak) peak = $2; cap = $3 }
		$1 == "h" { held += sample_s }
		END {
			line = sprintf("== %s load: peak %d/%d at once, held back %.0f s", label, peak, cap, held)
			if (n) line = line sprintf(", min idle %.1f/%d cores, min free %d MB (%d at the start), min free swap %d MB",
				idle / 10, cores, free, free0, swap)
			else line = line ", no load samples"
			print line sprintf(", %d s", now - start)
		}' "$GATE_STATS"
	rm -f "$GATE_STATS"
}

case $1 in
run)
	shift
	run "$@"
	;;
jobs) echo "$jobs" ;;
report) report "$2" ;;
*)
	sed -n '2,6p' "$0" >&2
	exit 2
	;;
esac
