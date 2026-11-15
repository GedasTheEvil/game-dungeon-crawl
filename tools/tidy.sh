#!/usr/bin/env bash
# Runs clang-tidy, one process per source through the load gate, and fails on any warning (plan dev-friction-review).
#
#   tools/tidy.sh changed|all SOURCES... -- TIDY_FLAGS...
#
# all: every source. changed: only what differs from the merge base with master (committed on a task branch,
# staged, unstaged or new): changed sources, and the sources that include a changed header, also through other
# headers (the build's .d files; a header's warnings show in any source that includes it). The tree is clean, so
# every warning printed is the run's own. Drops clang's "N warnings generated." lines (they count the warnings
# filtered out, e.g. in system headers).
#
# A source that came out clean is cached (plan check-speed-stage-breakdown): the key is a hash of the source, every
# repo header it includes (the build's .d file), .clang-tidy, the flags and the clang-tidy version, so a cache hit
# means the same input: a re-run after a header change (the fix loop) tidies only what the fix touched. The cache is in
# the git common dir (shared by the worktrees; entries unused for 30 days go). TIDY_CACHE=0 turns it off. A source
# without a .d file (not built yet) is never cached.
set -u
cd "$(dirname "$0")/.." || exit 2
mode=$1
cache=$(realpath "$(git rev-parse --git-common-dir)")/tidy-cache

# tidy.sh one SOURCE -- FLAGS...: one source through the cache and the gate; prints its warnings.
if [ "$mode" = one ]; then
	src=$2
	shift 3
	key=''
	dep=build/${src%.cpp}.d
	if [ "${TIDY_CACHE:-1}" != 0 ] && [ -f "$dep" ]; then
		# shellcheck disable=SC2046
		key=$({
			"${CLANG_TIDY:-clang-tidy}" --version
			echo "$*"
			cat .clang-tidy "$src"
			cat $(sed 's/[\\:]/ /g' "$dep" | tr ' ' '\n' | grep -E '^(src|tools|external)/|/(src|tools|external)/' | sort -u | sed "s|^$PWD/||" | while read -r f; do [ -f "$f" ] && echo "$f"; done)
		} | sha256sum | cut -c1-40)
		[ -e "$cache/$key" ] && exit 0
	fi
	out=$(./tools/load_gate.sh run "${CLANG_TIDY:-clang-tidy}" --quiet "$src" -- "$@" 2>&1 | sed '/^[0-9]* warnings\? generated\.$/d')
	[ -n "$out" ] && echo "$out"
	if [ -n "$key" ] && ! grep -qE '^[^ ]+:[0-9]+:[0-9]+: (warning|error):' <<<"$out"; then
		mkdir -p "$cache" && : >"$cache/$key"
	fi
	exit 0
fi
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
	headers=$(grep -E '\.h$' <<<"$changed")
	picked=()
	for src in "${sources[@]}"; do
		if grep -qxF "$src" <<<"$changed"; then
			picked+=("$src")
			continue
		fi
		[ -n "$headers" ] || continue
		# The headers it includes, also through other headers: the build's dependency file, else its own includes.
		dep="build/${src%.cpp}.d"
		if [ -f "$dep" ]; then
			# shellcheck disable=SC2046
			deps=$(realpath -m --relative-to=. $(sed 's/[\\:]/ /g' "$dep"))
		else
			deps=$(grep -oE '#include "[^"]+"' "$src" | cut -d'"' -f2 | sed "s|^|$(dirname "$src")/|" |
				xargs -r realpath -m --relative-to=.)
		fi
		if grep -qxFf <(echo "$headers") <<<"$deps"; then
			picked+=("$src")
		fi
	done
	sources=("${picked[@]+"${picked[@]}"}")
	echo "tidy: ${#sources[@]} changed source(s) since ${base:0:7} (make tidy-all checks every source)"
fi
[ -n "${CHECK_STAGE_NOTE:-}" ] && echo "${#sources[@]}" >"$CHECK_STAGE_NOTE"
[ ${#sources[@]} -eq 0 ] && exit 0

[ -d "$cache" ] && find "$cache" -type f -mtime +30 -delete
out=$(printf '%s\n' "${sources[@]}" | xargs -P $(($(./tools/load_gate.sh jobs) + 1)) -I{} \
	./tools/tidy.sh one {} -- "$@" 2>&1)
[ -n "$out" ] && echo "$out"
# Any warning or error fails the run; a header's warning repeats once per including source, count it once.
count=$(grep -oE '^[^ ]+:[0-9]+:[0-9]+: (warning|error):' <<<"$out" | sort -u | wc -l)
if [ "$count" -gt 0 ]; then
	echo "tidy: $count warning(s)"
	exit 1
fi
exit 0
