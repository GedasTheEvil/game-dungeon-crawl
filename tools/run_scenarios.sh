#!/bin/bash
# Runs scenario scripts through ./game. Usage: tools/run_scenarios.sh [scenario.txt ...]
# Uses Xvfb when available (HEADLESS=0 forces a real window). Output: tests/out/<name>/.
cd "$(dirname "$0")/.." || exit 2

scenarios=("$@")
[ ${#scenarios[@]} -eq 0 ] && scenarios=(tests/scenarios/*.txt)

runner=()
if [ "${HEADLESS:-1}" != 0 ] && command -v xvfb-run >/dev/null; then
	runner=(xvfb-run -a -s "-screen 0 1920x1080x24")
fi

failed=0
for scenario in "${scenarios[@]}"; do
	echo "== $scenario"
	"${runner[@]}" ./game "$scenario"
	code=$?
	[ $code -ne 0 ] && failed=$((failed + 1)) && echo "   exit $code"
done

echo "== $((${#scenarios[@]} - failed))/${#scenarios[@]} scenarios passed"
[ $failed -eq 0 ]
