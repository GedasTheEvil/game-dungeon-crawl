#!/usr/bin/env bash
# Checks that the source files are formatted, and that clang-format is stable on them
# (docs/plan/formatting-fixes.md).
#
#   tools/check_format.sh FILES...
#
# Formats each file twice, into temp copies (the files on disk stay as they are). Prints one line per file
#   * "not formatted": the first pass differs from the file (run `make format`)
#   * "unstable": the second pass differs from the first; clang-format flips the code between two layouts on every
#     run, so rewrite it (often a long multi-line trailing comment: move it above the line)
# and exits 1 if there is any.
set -u
cd "$(dirname "$0")/.." || exit 2
errors=0

for file in "$@"; do
	once=$(clang-format --assume-filename="$file" <"$file") || exit 2
	twice=$(clang-format --assume-filename="$file" <<<"$once") || exit 2
	if [ "$once" != "$twice" ]; then
		echo "$file: unstable"
		errors=1
	elif [ "$once" != "$(cat "$file")" ]; then
		echo "$file: not formatted"
		errors=1
	fi
done

exit $errors
