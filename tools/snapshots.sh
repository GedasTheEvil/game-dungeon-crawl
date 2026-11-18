#!/bin/bash
# Render snapshots (plan render-snapshots, docs/testing.md "Render snapshots"): runs tests/scenarios/snapshots/*.txt
# through tools/run_scenarios.sh (Xvfb, llvmpipe with one thread, no sound), gathers their screenshots into
# tests/out/snapshots/<scenario>-<shot>.png (the scenario's "snap_" prefix and the shot's number dropped), then diffs
# them against the references in tests/snapshots/ref/ with bin/snapdiff (diff PNGs in tests/out/snapshots/diff/).
# Usage: tools/snapshots.sh [--ref] [scenario.txt ...]
#   --ref: then write them as the new references (the table first shows what changed against the old ones), packed
#   smaller by optipng when it is installed.
#   scenarios: only these (default: all); the diff then covers only their images.
# SNAP_THRESHOLD, SNAP_TOLERANCE: bin/snapdiff's --threshold and --tolerance.
cd "$(dirname "$0")/.." || exit 2

ref=0
if [ "$1" = --ref ]; then
	ref=1
	shift
fi
scenarios=("$@")
[ ${#scenarios[@]} -eq 0 ] && scenarios=(tests/scenarios/snapshots/*.txt)
refdir=tests/snapshots/ref
out=tests/out/snapshots

./tools/run_scenarios.sh "${scenarios[@]}"
ran=$?

rm -rf "$out"
mkdir -p "$out/ref"
for s in "${scenarios[@]}"; do
	name=$(basename "$s" .txt)
	for png in "tests/out/$name"/[0-9][0-9][0-9]_*.png; do
		[ -e "$png" ] || continue
		shot=$(basename "$png" .png)
		cp "$png" "$out/${name#snap_}-${shot#[0-9][0-9][0-9]_}.png"
	done
	# The references of the scenarios run, so a subset run diffs only its own images.
	cp "$refdir/${name#snap_}"-*.png "$out/ref/" 2>/dev/null
done

args=()
[ -n "$SNAP_THRESHOLD" ] && args+=(--threshold "$SNAP_THRESHOLD")
[ -n "$SNAP_TOLERANCE" ] && args+=(--tolerance "$SNAP_TOLERANCE")
bin/snapdiff "${args[@]}" --diff "$out/diff" "$out/ref" "$out"
diffed=$?
rm -rf "$out/ref"

if [ $ref = 1 ]; then
	if [ $ran -ne 0 ]; then
		echo "== snapshots: a scenario failed, references not written"
		exit 1
	fi
	mkdir -p "$refdir"
	for s in "${scenarios[@]}"; do
		name=$(basename "$s" .txt)
		rm -f "$refdir/${name#snap_}"-*.png
	done
	cp "$out"/*.png "$refdir/"
	# Smaller files, the same pixels (stb writes them loosely packed): optipng when installed, no need otherwise.
	if command -v optipng >/dev/null; then
		for s in "${scenarios[@]}"; do
			name=$(basename "$s" .txt)
			printf '%s\0' "$refdir/${name#snap_}"-*.png
		done | xargs -0 -n 4 -P "$(tools/load_gate.sh jobs)" optipng -quiet -o2
	fi
	echo "== snapshots: $(find "$out" -maxdepth 1 -name '*.png' | wc -l) references written to $refdir (review: git diff --stat $refdir)"
	exit 0
fi
[ $ran -eq 0 ] && [ $diffed -eq 0 ]
