#!/usr/bin/env bash
# Runs the unit tests in parallel shards, each through the load gate (plan check-speed).
#
#   tools/unit_shards.sh UNIT [SHARDS]
#
# UNIT is the doctest binary (bin/unit). SHARDS (default: 6, at most the load gate's job cap) processes each run a window of the
# test cases in one seeded random order (--order-by=rand --rand-seed, --first/--last), so the slow level and model
# tests spread over the shards instead of sitting in one. Every test case runs exactly once, in one shard; a test case
# that writes a file uses its own path, so shards do not collide.
# Prints the output of every failed shard in full, then one summary line; exits 1 if any shard failed.
# `make unit-serial` runs them in one process (for a debugger, or doctest's own options).
set -u
cd "$(dirname "$0")/.." || exit 2
unit=$1
# 6: more shards load the models more often at once and end no sooner (plan check-speed, measured on 16 cores).
jobs=$(./tools/load_gate.sh jobs)
shards=${2:-$((jobs < 6 ? jobs : 6))}
seed=1

total=$("$unit" --count 2>/dev/null | sed -n 's/.*passing the current filters: \([0-9]*\).*/\1/p')
if [ -z "$total" ]; then
	echo "unit_shards: $unit --count failed" >&2
	exit 2
fi
[ "$shards" -gt "$total" ] && shards=$total
[ "$shards" -lt 1 ] && shards=1

out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
pids=()
for ((i = 0; i < shards; i++)); do
	first=$((i * total / shards + 1))
	last=$(((i + 1) * total / shards))
	./tools/load_gate.sh run "$unit" --order-by=rand --rand-seed=$seed --first=$first --last=$last \
		--no-version >"$out/$i" 2>&1 &
	pids+=($!)
done

failed=0
for ((i = 0; i < shards; i++)); do
	if ! wait "${pids[$i]}"; then
		failed=$((failed + 1))
		echo "=== unit shard $((i + 1))/$shards failed:"
		cat "$out/$i"
	fi
done

# One summary line. A shard's "test cases:" line counts every case (those outside its window as passed): only its
# failures count. Its "assertions:" line is its own.
awk -v total="$total" -v shards="$shards" -v failed="$failed" '
	/test cases:/ { for (i = 1; i <= NF; i++) if ($i == "failed") cf += $(i - 1) }
	/assertions:/ { for (i = 1; i <= NF; i++) { if ($i == "passed") ap += $(i - 1); if ($i == "failed") af += $(i - 1) } }
	END {
		printf "[doctest] test cases: %d, %d failed; assertions: %d passed, %d failed (%d shards, %d failed)\n",
			total, cf, ap, af, shards, failed
	}' "$out"/*
[ "$failed" -eq 0 ]
