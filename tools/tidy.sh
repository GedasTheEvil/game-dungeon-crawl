#!/usr/bin/env bash
# Runs clang-tidy, one process per source through the load gate, and fails on any warning (plan dev-friction-review).
#
#   tools/tidy.sh changed|all SOURCES... -- TIDY_FLAGS...
#
# all: every source. changed: only what differs from the merge base with master (committed on a task branch,
# staged, unstaged or new): changed sources, and the sources that include a changed header by name (directly; a
# header's warnings show in any source that includes it). The tree is clean, so every warning printed is the run's
# own. Drops clang's "N warnings generated." lines (they count the warnings filtered out, e.g. in system headers).
set -u
cd "$(dirname "$0")/.." || exit 2
mode=$1
shift
sources=()
while [ $# -gt 0 ] && [ "$1" != "--" ]; do
	sources+=("$1")
	shift
done
shift

if [ "$mode" = changed ]; then
	base=$(git merge-base HEAD master 2>/dev/null || echo HEAD)
	changed=$({ git diff --name-only "$base"; git ls-files --others --exclude-standard; } | sort -u)
	headers=$(grep -E '\.h$' <<<"$changed" | xargs -r -n1 basename | sort -u)
	picked=()
	for src in "${sources[@]}"; do
		if grep -qxF "$src" <<<"$changed"; then
			picked+=("$src")
			continue
		fi
		for h in $headers; do
			if grep -qE "#include \"([^\"]*/)?${h//./\\.}\"" "$src"; then
				picked+=("$src")
				break
			fi
		done
	done
	sources=("${picked[@]+"${picked[@]}"}")
	echo "tidy: ${#sources[@]} changed source(s) since ${base:0:7} (make tidy-all checks every source)"
fi
[ ${#sources[@]} -eq 0 ] && exit 0

out=$(printf '%s\n' "${sources[@]}" | xargs -P $(($(./tools/load_gate.sh jobs) + 1)) -I{} \
	./tools/load_gate.sh run "${CLANG_TIDY:-clang-tidy}" --quiet {} -- "$@" 2>&1 |
	sed '/^[0-9]* warnings\? generated\.$/d')
[ -n "$out" ] && echo "$out"
# Any warning or error fails the run; a header's warning repeats once per including source, count it once.
count=$(grep -oE '^[^ ]+:[0-9]+:[0-9]+: (warning|error):' <<<"$out" | sort -u | wc -l)
if [ "$count" -gt 0 ]; then
	echo "tidy: $count warning(s)"
	exit 1
fi
exit 0
