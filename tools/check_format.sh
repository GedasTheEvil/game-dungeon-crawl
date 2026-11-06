#!/usr/bin/env bash
# Checks that the source files are formatted, and that clang-format is stable on them (plan formatting-fixes).
#
#   tools/check_format.sh [--changed] FILES...
#
# Formats each file twice, into temp copies (the files on disk stay as they are). Prints one line per file
#   * "not formatted": the first pass differs from the file (run `make format`)
#   * "unstable": the second pass differs from the first; clang-format flips the code between two layouts on every
#     run, so rewrite it (often a long multi-line trailing comment: move it above the line)
# and exits 1 if there is any.
#
# --changed (`make check`, plan check-speed): only the FILES that differ from the merge base with master (committed
# on a task branch, staged, unstaged or new). `make check` runs this right after `make format`, so a file that is
# not formatted cannot show there: only "unstable" can, and only in a file that changed.
# The files are checked in parallel, as many at once as the load gate's job cap (`tools/load_gate.sh jobs`; not
# through the gate itself: its idle-core sample per job would cost more than a clang-format run).
set -u
cd "$(dirname "$0")/.." || exit 2

if [ "${1:-}" = --one ]; then
	file=$2
	once=$(clang-format --assume-filename="$file" <"$file") || exit 2
	twice=$(clang-format --assume-filename="$file" <<<"$once") || exit 2
	if [ "$once" != "$twice" ]; then
		echo "$file: unstable"
		exit 1
	elif [ "$once" != "$(cat "$file")" ]; then
		echo "$file: not formatted"
		exit 1
	fi
	exit 0
fi

files=("$@")
if [ "${1:-}" = --changed ]; then
	shift
	base=$(git merge-base HEAD master 2>/dev/null || echo HEAD)
	changed=$({ git diff --name-only "$base"; git ls-files --others --exclude-standard; } | sort -u)
	files=()
	for file in "$@"; do
		grep -qxF "$file" <<<"$changed" && files+=("$file")
	done
fi
[ ${#files[@]} -eq 0 ] && exit 0

# xargs exits 123 when any check failed, 2 stays a clang-format error.
printf '%s\0' "${files[@]}" | xargs -0 -n 1 -P "$(./tools/load_gate.sh jobs)" "$0" --one
case $? in
0) exit 0 ;;
123) exit 1 ;;
*) exit 2 ;;
esac
