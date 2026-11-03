#!/bin/bash
# Profiles one scenario in bin/game with perf: silent (Xvfb, --no-sound), bounded, one llvmpipe thread as in
# tools/run_scenarios.sh. Prints the wall time, the share per library and the top functions (self and with callees).
# Usage: tools/profile_game.sh [-t seconds (default 600)] [-n lines (default 25)] [-o file.perf] scenario.txt
# Needs perf with kernel.perf_event_paranoid <= 1 (docs/performance-testing.md). The data stays in the -o file
# (default /tmp/profile_game.perf) for `perf report -i`.
cd "$(dirname "$0")/.." || exit 2

limit=600 lines=25 data=/tmp/profile_game.perf
while getopts "t:n:o:" opt; do
	case $opt in
	t) limit=$OPTARG ;;
	n) lines=$OPTARG ;;
	o) data=$OPTARG ;;
	*)
		sed -n '2,6p' "$0" >&2
		exit 2
		;;
	esac
done
shift $((OPTIND - 1))
[ $# -eq 1 ] || {
	sed -n '2,6p' "$0" >&2
	exit 2
}
command -v perf >/dev/null || {
	echo "perf missing: sudo apt install linux-tools-generic" >&2
	exit 2
}

start=$(date +%s.%N)
LP_NUM_THREADS=${LP_NUM_THREADS:-1} xvfb-run -a -s "-screen 0 800x600x24" \
	timeout -k 5 "$limit" perf record -g -F 499 -o "$data" bin/game --no-sound "$1" >/dev/null 2>&1
code=$?
name=$(basename "$1" .txt)
awk -v s="$start" -v e="$(date +%s.%N)" -v c="$code" 'BEGIN { printf "== wall %.1f s, exit %d\n", e - s, c }'
tail -n 1 "tests/out/$name/result.txt" 2>/dev/null

report() { perf report -i "$data" --stdio -g none "$@" 2>/dev/null | grep '%' | grep -v '0x0000' | head -n "$lines"; }
echo "== by library"
report --no-children --sort dso
echo "== by function, self"
report --no-children --sort sym
echo "== by function, with callees"
report --children --sort sym
