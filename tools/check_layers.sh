#!/usr/bin/env bash
# Checks that the shared libraries stay apart from the game (docs/plan/solved/layered-build.md).
#
#   tools/check_layers.sh NAME FILES... [-- NAME FILES...]...
#
# For each library NAME and its FILES (.cpp with their matching .h, and header-only .h files):
#   * quoted includes must be headers of the same library, of the library named "base", or under external/
#   * no SDL, no Game()
#   * the libraries named "base" and "level" have no GL either
# The base library (named first) sits under the others: its headers may be included by every library.
# Prints one line per violation and exits 1 if there is any.
set -u
cd "$(dirname "$0")/.." || exit 2
root=$(pwd)
errors=0
base_headers=()

check_library() {
	local name=$1
	shift
	local files=() allowed=("${base_headers[@]}")
	for source in "$@"; do
		files+=("$source")
		if [ "${source%.h}" != "$source" ]; then
			allowed+=("$root/$source")
			continue
		fi
		local header=${source%.cpp}.h
		if [ -f "$header" ]; then
			files+=("$header")
			allowed+=("$root/$header")
		fi
	done
	for file in "${files[@]}"; do
		local dir
		dir=$(dirname "$file")
		while IFS=: read -r line include; do
			local target
			target=$(realpath -m "$dir/$include")
			case "$target" in "$root"/external/*) continue ;; esac
			local ok=0
			for header in "${allowed[@]}"; do
				[ "$target" = "$header" ] && ok=1
			done
			if [ $ok -eq 0 ]; then
				echo "$file:$line: $name library includes \"$include\", which is not part of it"
				errors=$((errors + 1))
			fi
		done < <(grep -n '^#include "' "$file" | sed 's/^\([0-9]*\):#include "\([^"]*\)".*/\1:\2/')
		local forbidden='#include <SDL|\bGame\(\)'
		case "$name" in base | level) forbidden="$forbidden|#include <GL" ;; esac
		while IFS= read -r hit; do
			echo "$file:${hit%%:*}: $name library uses ${hit#*:}"
			errors=$((errors + 1))
		done < <(grep -nE "$forbidden" "$file")
	done
	[ "$name" = base ] && base_headers=("${allowed[@]}")
}

args=()
for arg in "$@" --; do
	if [ "$arg" = -- ]; then
		[ ${#args[@]} -gt 0 ] && check_library "${args[@]}"
		args=()
	else
		args+=("$arg")
	fi
done

if [ $errors -gt 0 ]; then
	echo "check_layers: $errors violation(s)"
	exit 1
fi
echo "check_layers: OK"
