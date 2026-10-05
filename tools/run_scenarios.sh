#!/bin/bash
# Runs scenario scripts through ./game, JOBS at a time (default: nproc - 4, at least 1, so four cores stay free).
# Usage: tools/run_scenarios.sh [scenario.txt ...]
# Uses Xvfb when available, one server per run. HEADLESS=0 (or no Xvfb) runs one real window at a time, every frame
# drawn (SCENARIO_DRAW_ALL). Output: tests/out/<name>/.
cd "$(dirname "$0")/.." || exit 2

scenarios=("$@")
[ ${#scenarios[@]} -eq 0 ] && scenarios=(tests/scenarios/*.txt)

jobs=${JOBS:-$(($(nproc) - 6))}
[ "$jobs" -lt 1 ] && jobs=1
headless=0
if [ "${HEADLESS:-1}" != 0 ] && command -v xvfb-run >/dev/null; then
	headless=1
else
	export SCENARIO_DRAW_ALL=1
	jobs=1
fi

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# $1 scenario, $2 its index (a distinct display number, so parallel xvfb-run -a does not race for the same one)
run_one() {
	local out code
	if [ $headless = 1 ]; then
		out=$(xvfb-run -a -n $((200 + $2 * 3)) -s "-screen 0 800x600x24" ./game "$1" 2>&1)
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
	while [ "$(jobs -rp | wc -l)" -ge "$jobs" ]; do
		wait -n
	done
	run_one "${scenarios[$i]}" "$i" &
done
wait

failed=0
for i in "${!scenarios[@]}"; do
	[ "$(cat "$tmp/$i.code" 2>/dev/null)" = 0 ] || failed=$((failed + 1))
done

echo "== $((${#scenarios[@]} - failed))/${#scenarios[@]} scenarios passed"
[ $failed -eq 0 ]
